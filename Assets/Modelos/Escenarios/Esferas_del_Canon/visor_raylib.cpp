#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <string>
static const char* N[]={"tramo_pista_1","pared_izquierda_1","pared_derecha_1","tramo_pista_2","pared_izquierda_2","pared_derecha_2","tramo_pista_3","pared_izquierda_3","pared_derecha_3","tramo_pista_4","pared_izquierda_4","pared_derecha_4","puente_roto_1","puente_roto_2","puente_roto_3","rampa_atajo","arco_natural","mesa_lejana","cactus","roca_caida","banderin_checkpoint","arco_salida","arco_meta","esfera_piedra","aro_jugador","suelo_desertico"};
static Model M[26]{};
static float PX[53],PZ[53],PS[53];
static void InitPista(){
 const float x[]={0,0,3,8,9,4,-3,-8,-9,-4,3,7,3,0};int count=0;
 for(int i=0;i<13;i++)for(int k=0;k<4;k++){
  float t=k/4.f,t2=t*t,t3=t2*t,p0=x[i>0?i-1:0],p1=x[i],p2=x[i+1],p3=x[i+2<14?i+2:13];
  PX[count]=.5f*(2*p1+(-p0+p2)*t+(2*p0-5*p1+4*p2-p3)*t2+(-p0+3*p1-3*p2+p3)*t3);
  PZ[count]=-10.f*(i+t);count++;
 }
 PX[52]=0;PZ[52]=-130;PS[0]=0;
 for(int i=1;i<53;i++)PS[i]=PS[i-1]+hypotf(PX[i]-PX[i-1],PZ[i]-PZ[i-1]);
}
static Vector3 Point(float s,float lateral,float h=0){
 s=fmaxf(0,fminf(PS[52],s));int i=0;while(i<51&&PS[i+1]<s)i++;
 float dx=PX[i+1]-PX[i],dz=PZ[i+1]-PZ[i],len=hypotf(dx,dz),t=(s-PS[i])/len;
 return {PX[i]+dx*t-dz/len*lateral,-.12f*s+h,PZ[i]+dz*t+dx/len*lateral};
}
static void Part(int i,Vector3 p={0,0,0},Vector3 scale={1,1,1},Color color=WHITE,float yaw=0){DrawModelEx(M[i],p,{0,1,0},yaw,scale,color);}
static void Scene(int mode){
 if(mode>=10){Part(mode-10);return;}
 Part(25);
 for(int i=0;i<4;i++)for(int k=0;k<3;k++)Part(i*3+k);
 for(int i=12;i<=16;i++)Part(i);
 for(int s: {28,58,88})for(int side: {-1,1})Part(20,Point((float)s,side*3.9f));
 for(int i=0;i<10;i++){
  float s=10.f+i*11.7f,l=i%2?1.8f:-1.6f;
  Part(i%3?19:18,Point(s,l));
  Part(18,Point(s+3.5f,(i%2?1:-1)*5.3f,7.0f));
 }
 for(int i=0;i<10;i++){
  float z=-i*14.f,x=(i%2?-1.f:1.f)*(20.f+(i*7)%10);Part(17,{x,-.12f*(-z)-2,z},{1,1+(i%4)*.15f,1});
 }
 float s0=.8f,s1=120;
 for(int j=0;j<2;j++){
  float s=j?s1:s0;Vector3 c=Point(s,0);Vector3 d=Point(s,1);
  float yaw=atan2f(d.z-c.z,d.x-c.x)*RAD2DEG;Part(j?22:21,c,{1,1,1},WHITE,yaw);
 }
 if(mode!=4){
  float starts[]={2,3.2f,4.5f,5.8f},ls[]={-3.3f,-1.1f,1.1f,3.3f};
  for(int i=0;i<4;i++){
   float s=mode==3?75.f+i*1.25f:mode==2?94.f+i*1.25f:starts[i];Vector3 p=Point(s,ls[i],1);
   Part(23,p);Part(24,Point(s,ls[i],.07f),{1,1,1},i==0?RED:i==1?BLUE:i==2?GREEN:YELLOW);
   DrawCube({p.x,p.y+1.7f,p.z},.45f,.8f,.45f,i==0?RED:i==1?BLUE:i==2?GREEN:YELLOW);
  }
 }
}
static Camera3D ModelCam(int i){
 BoundingBox b=GetModelBoundingBox(M[i]);Vector3 center=Vector3Scale(Vector3Add(b.min,b.max),.5f),size=Vector3Subtract(b.max,b.min);
 float radius=fmaxf(size.x,fmaxf(size.y,size.z));Vector3 dir=Vector3Normalize({.8f,.8f,1.35f});
 if(i<12||i==25)dir=Vector3Normalize({.35f,1.8f,1.9f});
 return {Vector3Add(center,Vector3Scale(dir,radius*1.55f+1.2f)),center,{0,1,0},37,CAMERA_PERSPECTIVE};
}
static void Render(Camera3D camera,int mode,bool hud){BeginDrawing();ClearBackground({232,180,130,255});BeginMode3D(camera);Scene(mode);EndMode3D();if(hud){DrawRectangle(0,0,1280,64,{71,44,48,225});DrawText("ESFERAS DEL CANON / Modelos GLB",20,8,23,RAYWHITE);DrawText("1 Salida | 2 Rampa | 3 Grietas | 4 Paisaje | Flechas: mover camara",20,39,16,RAYWHITE);}EndDrawing();}
static void Capture(const char* name,Camera3D cam,int mode){Render(cam,mode,false);TakeScreenshot((std::string("Vistas/")+name+".png").c_str());}
int main(int argc,char** argv){
 std::string cwd=GetWorkingDirectory();bool capture=argc>1&&std::string(argv[1])=="--capturar",verify=argc>1&&std::string(argv[1])=="--verificar";
 SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_MSAA_4X_HINT);InitWindow(capture?1600:1280,capture?1000:800,"Esferas del Canon - modelos GLB");
 if(!IsWindowReady())return 1;ChangeDirectory(cwd.c_str());rlSetClipPlanes(.2,400);InitPista();
 int loaded=0;for(int i=0;i<26;i++){
  std::string path=std::string("GLB/")+N[i]+".glb";if(!FileExists(path.c_str())){fprintf(stderr,"Falta %s\n",path.c_str());break;}
  M[i]=LoadModel(path.c_str());loaded++;if(!M[i].meshCount)break;printf("VALIDADO %s meshes=%d\n",N[i],M[i].meshCount);
 }
 if(loaded!=26){for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 2;}
 if(capture){
  Vector3 p=Point(4,0);Capture("Salida",{{p.x+15,p.y+21,p.z+22},{p.x,p.y,p.z-11},{0,1,0},57,CAMERA_PERSPECTIVE},1);
  p=Point(92,0);Capture("Rampa",{{p.x+12,p.y+15,p.z+14},{p.x,p.y,p.z-5},{0,1,0},58,CAMERA_PERSPECTIVE},2);
  p=Point(76,0);Capture("Grietas",{{p.x+11,p.y+15,p.z+16},{p.x,p.y,p.z-5},{0,1,0},57,CAMERA_PERSPECTIVE},3);
  Capture("Vista_general",{{44,60,35},{0,-8,-64},{0,1,0},55,CAMERA_PERSPECTIVE},4);
  p=Point(2,0);Capture("Camara_del_juego",{{p.x,p.y+8,p.z+9},{p.x,p.y,p.z},{0,1,0},50,CAMERA_PERSPECTIVE},1);
  for(int i=0;i<26;i++)Capture(N[i],ModelCam(i),i+10);
 }else{SetTargetFPS(60);int mode=1,frame=0;float s=4;while(!WindowShouldClose()&&(!verify||frame++<10)){
  if(IsKeyPressed(KEY_ONE))mode=1;if(IsKeyPressed(KEY_TWO))mode=2;if(IsKeyPressed(KEY_THREE))mode=3;if(IsKeyPressed(KEY_FOUR))mode=4;
  if(IsKeyDown(KEY_RIGHT))s=fminf(120,s+GetFrameTime()*15);if(IsKeyDown(KEY_LEFT))s=fmaxf(0,s-GetFrameTime()*15);
  Vector3 p=Point(s,0);Camera3D cam={{p.x+8,p.y+13,p.z+16},{p.x,p.y,p.z-7},{0,1,0},53,CAMERA_PERSPECTIVE};Render(cam,mode,true);
 }}for(int i=0;i<loaded;i++)UnloadModel(M[i]);CloseWindow();return 0;
}
