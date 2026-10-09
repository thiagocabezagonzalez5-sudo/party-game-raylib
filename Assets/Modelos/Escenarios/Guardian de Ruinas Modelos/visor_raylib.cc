#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cstdio>
#include <string>
static const char* N[]={"terreno_selva","plaza_losas","muro_lateral","escalinata","templo_fondo","muro_fondo","portal_marco","runa","velo_portal","linea_meta","columna_entera","aviso_columna","columna_caida","estatua_caida","arbol_selva","arbusto","orbe_azul","orbe_devuelto","escudo_guardian","escudo_embestida"};
static Model M[20]{};static std::string ROOT;
static void Part(int i,Vector3 p={0,0,0},Vector3 s={1,1,1}){DrawModelEx(M[i],p,{0,1,0},0,s,WHITE);}
static Camera3D Cam(int mode){
 if(mode==3)return {{11,14,16},{0,1,-6},{0,1,0},48,CAMERA_PERSPECTIVE};
 if(mode==2)return {{0,13,12},{0,1,-3},{0,1,0},50,CAMERA_PERSPECTIVE};
 return {{0,19,17},{0,0,-2.5f},{0,1,0},50,CAMERA_PERSPECTIVE};
}
static void Scene(int mode){
 if(mode>=10){Part(mode-10);return;}
 Part(0);Part(1);Part(4,{0,0,-18});
 Part(3,{0,0,-13.3f});
 for(int s:{-1,1}){
  Part(5,{s*6.4f,0,-12.7f});
  for(int k=0;k<8;k++)Part(2,{s*9.1f,0,-11+k*2.6f});
  Part(13,{s*7.4f,0,7.6f});
  for(int k=0;k<5;k++)Part(15,{s*8.4f,0,-9+k*4.2f});
  for(int k=0;k<4;k++)Part(14,{s*(19+k*2.1f),0,-12+k*6.f},{1+.08f*k,1+.08f*k,1+.08f*k});
 }
 for(int k=0;k<8;k++)Part(14,{-28+k*8.f,0,-31.f-(k%3)*2.f},{1.2f,1.2f,1.2f});
 Part(6,{0,0,-12.7f});Part(8,{0,0,-12.5f});Part(9,{0,0,-12.1f});
 for(int s:{-1,1})for(float y:{1.2f,2.5f,3.8f})Part(7,{s*3.f,y,-12.11f});
 float cx[]={-5.5f,5.5f,-2.8f,2.8f,-6.2f,6.2f};
 float cz[]={-3.f,-3.f,1.2f,1.2f,4.2f,4.2f};
 for(int c=0;c<6;c++){
  bool fallen=mode==2&&c==3;
  Part(fallen?12:10,{cx[c],0,cz[c]});
  if(mode==2&&c==4)Part(11,{cx[c],0,cz[c]});
 }
 DrawCube({0,.7f,-10.6f},.65f,1.4f,.65f,{120,220,255,255});
 for(int i=0;i<3;i++)DrawCube({-3.4f+i*3.4f,.7f,6.f},.65f,1.4f,.65f,{255,170,60,255});
 float shieldZ=mode==3?-7.4f:-9.5f;
 Part(mode==3?19:18,{0,1,shieldZ});
 Part(16,{-1.1f,.9f,1.8f});Part(16,{2.2f,.9f,-1.f});
 if(mode>=2)Part(17,{.3f,.9f,-6.f});
}
static Camera3D ModelCam(int i){
 BoundingBox b=GetModelBoundingBox(M[i]);Vector3 c=Vector3Scale(Vector3Add(b.min,b.max),.5f);
 Vector3 d=Vector3Subtract(b.max,b.min);float r=fmaxf(d.x,fmaxf(d.y,d.z));
 Vector3 dir=Vector3Normalize({.7f,1.f,1.5f});if(i==4||i==6||i==8||i>=18)dir=Vector3Normalize({.35f,.5f,1.6f});
 if(i==0||i==1||i==3)dir=Vector3Normalize({.6f,1.8f,1.3f});
 return {Vector3Add(c,Vector3Scale(dir,r*1.45f+1)),c,{0,1,0},42,CAMERA_PERSPECTIVE};
}
static void Render(Camera3D c,int mode,bool hud){
 BeginDrawing();ClearBackground({26,52,40,255});BeginMode3D(c);Scene(mode);EndMode3D();
 if(hud){DrawRectangle(0,0,1280,68,{28,50,39,232});DrawText("GUARDIAN DE RUINAS / Modelos GLB",20,8,24,RAYWHITE);DrawText("1 Plaza | 2 Columna caida | 3 Escudo en embestida",20,42,16,RAYWHITE);}EndDrawing();
}
static void Shot(const char* name,Camera3D c,int mode){Render(c,mode,false);TakeScreenshot((ROOT+"/Vistas/"+name+".png").c_str());}
int main(int argc,char**argv){
 ROOT=argc>2?argv[2]:GetWorkingDirectory();bool cap=argc>1&&std::string(argv[1])=="--capturar",verify=argc>1&&std::string(argv[1])=="--verificar";
 SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_MSAA_4X_HINT);InitWindow(cap?1600:1280,cap?1000:800,"Guardian de Ruinas - GLB");if(!IsWindowReady())return 1;
 ChangeDirectory(ROOT.c_str());rlSetClipPlanes(.2,240);
 int loaded=0;for(int i=0;i<20;i++){
  std::string p=std::string("GLB/")+N[i]+".glb";if(!FileExists(p.c_str())){fprintf(stderr,"Falta %s\n",p.c_str());break;}
  M[i]=LoadModel(p.c_str());if(!M[i].meshCount)break;loaded++;printf("VALIDADO %s meshes=%d\n",N[i],M[i].meshCount);
 }
 if(loaded!=20){for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 2;}
 if(cap){Shot("Plaza_del_portal",Cam(1),1);Shot("Columna_caida",Cam(2),2);Shot("Escudo_embestida",Cam(3),3);for(int i=0;i<20;i++)Shot(N[i],ModelCam(i),i+10);}
 else{SetTargetFPS(60);int mode=1,frames=0;while(!WindowShouldClose()&&(!verify||frames++<10)){if(IsKeyPressed(KEY_ONE))mode=1;if(IsKeyPressed(KEY_TWO))mode=2;if(IsKeyPressed(KEY_THREE))mode=3;Render(Cam(mode),mode,true);}}
 for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 0;
}
