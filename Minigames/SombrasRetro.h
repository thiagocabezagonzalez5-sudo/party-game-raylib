#pragma once

#include "raylib.h"

#include <cmath>


//==================================================
// SOMBRAS RETRO DE BAJO COSTO
//==================================================
//
// En vez de shadow maps usamos manchas planas de pocos triangulos.
// Es mucho mas barato para un party game con muchas primitivas y,
// ademas, encaja mejor con el look retro/low-poly del proyecto.
//==================================================

inline float LimitarSombraRetro(float valor,float minimo,float maximo){if(valor<minimo)return minimo;if(valor>maximo)return maximo;return valor;}

inline void DibujarSombraRetroCircular(Vector3 posicionObjeto,float radioX,float radioZ,float alturaSuelo=0.018f,float alphaBase=0.27f){if(radioX<=0.025f||radioZ<=0.025f)return;float altura=posicionObjeto.y-alturaSuelo;if(altura<0.0f)altura=0.0f;float factorAltura=LimitarSombraRetro(altura/7.0f,0.0f,1.0f);float desplazamiento=LimitarSombraRetro(altura*0.085f,0.0f,0.62f);float expansion=1.0f+factorAltura*0.32f;float alpha=alphaBase*(1.0f-factorAltura*0.58f);Vector3 centro={posicionObjeto.x+desplazamiento*0.72f,alturaSuelo,posicionObjeto.z+desplazamiento*0.45f};const int LADOS=8;Color color=Fade(BLACK,alpha);for(int i=0;i<LADOS;i++){float a0=2.0f*PI*(float)i/(float)LADOS;float a1=2.0f*PI*(float)(i+1)/(float)LADOS;Vector3 p0={centro.x+std::cos(a0)*radioX*expansion,alturaSuelo,centro.z+std::sin(a0)*radioZ*expansion};Vector3 p1={centro.x+std::cos(a1)*radioX*expansion,alturaSuelo,centro.z+std::sin(a1)*radioZ*expansion};DrawTriangle3D(centro,p1,p0,color);}}

inline void DibujarSombraRetroRectangular(Vector3 posicionObjeto,float ancho,float largo,float rotacionRad=0.0f,float alturaSuelo=0.018f,float alphaBase=0.24f){if(ancho<=0.04f||largo<=0.04f)return;float altura=posicionObjeto.y-alturaSuelo;if(altura<0.0f)altura=0.0f;float factorAltura=LimitarSombraRetro(altura/7.0f,0.0f,1.0f);float desplazamiento=LimitarSombraRetro(altura*0.085f,0.0f,0.62f);float expansion=1.0f+factorAltura*0.22f;float alpha=alphaBase*(1.0f-factorAltura*0.58f);float hx=ancho*0.5f*expansion;float hz=largo*0.5f*expansion;float c=std::cos(rotacionRad);float s=std::sin(rotacionRad);Vector3 centro={posicionObjeto.x+desplazamiento*0.72f,alturaSuelo,posicionObjeto.z+desplazamiento*0.45f};Vector3 e[4]={{-hx,0.0f,-hz},{hx,0.0f,-hz},{hx,0.0f,hz},{-hx,0.0f,hz}};for(int i=0;i<4;i++){float x=e[i].x,z=e[i].z;e[i].x=centro.x+x*c-z*s;e[i].y=alturaSuelo;e[i].z=centro.z+x*s+z*c;}Color color=Fade(BLACK,alpha);DrawTriangle3D(e[0],e[2],e[1],color);DrawTriangle3D(e[0],e[3],e[2],color);}

inline bool EsSuperficieBaseRetro(float ancho,float alto,float largo){return alto<=0.82f&&ancho*largo>=18.0f;}

inline void DibujarCuboConSombraRetro(Vector3 posicion,float ancho,float alto,float largo,Color color){if(!EsSuperficieBaseRetro(ancho,alto,largo)&&ancho>=0.16f&&largo>=0.16f&&alto>=0.08f)DibujarSombraRetroRectangular(posicion,ancho*0.92f,largo*0.92f);DrawCube(posicion,ancho,alto,largo,color);}

inline void DibujarEsferaConSombraRetro(Vector3 posicion,float radio,Color color){if(radio>=0.10f)DibujarSombraRetroCircular(posicion,radio*0.92f,radio*0.72f);DrawSphere(posicion,radio,color);}

inline void DibujarCilindroConSombraRetro(Vector3 posicion,float radioSuperior,float radioInferior,float alto,int lados,Color color){float radio=radioSuperior>radioInferior?radioSuperior:radioInferior;bool esBaseGrande=alto<=0.82f&&radio>=2.35f;if(!esBaseGrande&&radio>=0.10f&&alto>=0.07f)DibujarSombraRetroCircular(posicion,radio*0.92f,radio*0.72f);DrawCylinder(posicion,radioSuperior,radioInferior,alto,lados,color);}

inline void DibujarModeloExConSombraRetro(Model modelo,Vector3 posicion,Vector3 ejeRotacion,float anguloRotacion,Vector3 escala,Color tinte){float radioX=std::fabs(escala.x)*0.52f;float radioZ=std::fabs(escala.z)*0.44f;if(radioX<0.24f)radioX=0.24f;if(radioZ<0.20f)radioZ=0.20f;DibujarSombraRetroCircular(posicion,radioX,radioZ);DrawModelEx(modelo,posicion,ejeRotacion,anguloRotacion,escala,tinte);}

#ifdef SOMBRAS_RETRO_AUTOMATICAS
// Variadicas para aceptar llamadas con Vector3{ x, y, z } sin que las
// comas del inicializador confundan al preprocesador.
#define DrawCube(...) DibujarCuboConSombraRetro(__VA_ARGS__)
#define DrawSphere(...) DibujarEsferaConSombraRetro(__VA_ARGS__)
#define DrawCylinder(...) DibujarCilindroConSombraRetro(__VA_ARGS__)
#define DrawModelEx(...) DibujarModeloExConSombraRetro(__VA_ARGS__)
#endif
