"""Modelos originales GLB 2.0 del minijuego Laberinto Jade (raylib)."""
from geometria_glb import *
import geometria_glb as glb

glb.PALETTE.update({
 'piedra':'#978971','piedra_clara':'#b2a389','piedra_oscura':'#534b43',
 'losa':'#776b5c','arena':'#a5977c','fisura':'#4a423b',
 'jade':'#3dac84','jade_claro':'#83dfb7','jade_oscuro':'#226850',
 'oro':'#e4b94e','oro_oscuro':'#9b7134','tierra':'#6d4c3c',
 'brasa':'#e87237','fuego':'#ffc875','sombra':'#101319',
 'muro_fondo':'#423e39','COLOR_DINAMICO':'#ffffff',
 'BOMBILLAS':'#fff3bf'
})

def losa():
 m=mesh()
 box(m,(0,-.31,0),(12,.60,12),'piedra_oscura')
 box(m,(0,-.012,0),(11,.036,11),'losa')
 for i in range(-5,6):
  for j in range(-5,6):
   if (i+j)%2==0:
    box(m,(i,.011,j),(.94,.014,.94),'arena')
   if (i*3+j*7)%13==0:
    beam(m,(i-.27,.02,j-.24),(i+.10,.021,j+.04),.009,'fisura',5)
 for sign in [-1,1]:
  box(m,(0,-.014,sign*5.79),(12,.095,.40),'jade_oscuro')
  box(m,(sign*5.79,-.014,0),(.40,.095,12),'jade_oscuro')
  box(m,(0,.038,sign*5.78),(11.92,.020,.09),'jade_claro')
  box(m,(sign*5.78,.038,0),(.09,.020,11.92),'jade_claro')
 for x in range(-5,6):
  for z in [-5.79,5.79]:
   box(m,(x,.061,z),(.055,.018,.15),'oro')
  for xx in [-5.79,5.79]:
   box(m,(xx,.061,x),(.15,.018,.055),'oro')
 for x in [-5.75,5.75]:
  for z in [-5.75,5.75]:
   cylinder(m,(x,.05,z),.20,.12,'oro_oscuro',8)
   ellipsoid(m,(x,.19,z),(.14,.14,.14),'jade',8,5)
 return m

def banda_jugador():
 m=mesh();box(m,(0,0,0),(11.88,.27,.07),'COLOR_DINAMICO')
 for x in range(-5,6):
  box(m,(x,.15,.048),(.11,.055,.028),'oro')
 return m

def muro():
 m=mesh()
 # Colision logica ocupa una celda entera; nada sobresale en X/Z.
 box(m,(0,.285,0),(.97,.57,.97),'piedra')
 box(m,(0,.555,0),(.98,.045,.98),'piedra_clara')
 for x in [-.44,.44]:
  box(m,(x,.29,0),(.035,.48,.82),'piedra_oscura')
 for z in [-.44,.44]:
  box(m,(0,.29,z),(.82,.48,.035),'piedra_oscura')
 for x,z in [(-.32,-.30),(.31,.31)]:
  box(m,(x,.586,z),(.10,.012,.09),'oro_oscuro')
 beam(m,(-.24,.601,.18),(-.02,.604,.17),.007,'fisura',5)
 beam(m,(-.02,.604,.17),(.12,.606,.28),.007,'fisura',5)
 return m

def glifo():
 m=mesh()
 # Se coloca sobre el muro en Y=.61.
 torus(m,(0,.011,0),.155,.014,'oro',20,5)
 for a,b in [((-.16,.013,0),(.16,.013,0)),((0,.013,-.16),(0,.013,.16)),
             ((-.10,.014,-.10),(.10,.014,.10))]:
  beam(m,a,b,.011,'oro',6)
 ellipsoid(m,(0,.024,0),(.045,.022,.045),'jade_claro',8,4)
 return m

def agujero():
 m=mesh()
 # Marca visual oscura; el radio de caida real sigue siendo .34.
 cylinder(m,(0,.013,0),.45,.020,'tierra',24)
 cylinder(m,(0,.038,0),.366,.012,'sombra',24)
 torus(m,(0,.048,0),.39,.029,'oro_oscuro',28,5)
 for a in [0,1.57,3.14,4.71]:
  x,z=.49*math.cos(a),.49*math.sin(a)
  beam(m,(x,.053,z),(x*1.25,.056,z*1.25),.012,'fisura',5)
 return m

def salida():
 m=mesh();cylinder(m,(0,0,0),.39,.023,'jade_oscuro',24)
 torus(m,(0,.024,0),.37,.019,'jade_claro',24,5)
 for i in range(4):
  a=TAU*i/4
  box(m,(math.cos(a)*.28,.033,math.sin(a)*.28),(.07,.012,.07),'oro')
 return m

def control():
 m=mesh();cylinder(m,(0,0,0),.42,.035,'COLOR_DINAMICO',24)
 torus(m,(0,.037,0),.39,.026,'oro_oscuro',24,5)
 cylinder(m,(0,.037,0),.26,.030,'piedra_oscura',16)
 cylinder(m,(0,.067,0),.052,.47,'piedra_clara',8,r_top=.042)
 ellipsoid(m,(0,.60,0),(.12,.12,.12),'COLOR_DINAMICO',12,6)
 return m

def altar():
 m=mesh()
 # Pie dentro de .85 x .85; meta ocupa el centro de una celda.
 box(m,(0,.085,0),(.84,.17,.84),'piedra_clara')
 box(m,(0,.19,0),(.72,.07,.72),'piedra_oscura')
 box(m,(0,.29,0),(.54,.17,.54),'oro_oscuro')
 box(m,(0,.39,0),(.49,.04,.49),'oro')
 for x in [-.34,.34]:
  for z in [-.34,.34]:
   cylinder(m,(x,.18,z),.04,.18,'oro',8)
 torus(m,(0,.465,0),.18,.024,'oro',24,6)
 ellipsoid(m,(0,.65,0),(.17,.20,.17),'BOMBILLAS',16,9)
 return m

def esfera():
 m=mesh()
 # Centro en origen: colocar en Y=.31; radio maximo .28.
 ellipsoid(m,(0,0,0),(.28,.28,.28),'jade',24,12)
 torus(m,(0,.0,0),.264,.008,'jade_oscuro',32,5,plane='xy')
 for i in range(6):
  a=TAU*i/6
  ellipsoid(m,(.20*math.cos(a),.15,.20*math.sin(a)),(.035,.028,.035),'jade_claro',8,4)
 ellipsoid(m,(-.09,.13,-.09),(.067,.035,.065),'BOMBILLAS',10,5)
 return m

def trampa():
 m=mesh()
 # Flecha de advertencia a ras de suelo, geometria alargada en X.
 box(m,(-.075,.015,0),(.50,.022,.095),'COLOR_DINAMICO')
 tri(m,(.30,.028,0),(.03,.028,-.23),(.03,.028,.23),'COLOR_DINAMICO')
 tri(m,(.03,.006,.23),(.03,.006,-.23),(.30,.006,0),'COLOR_DINAMICO')
 return m

def boquilla():
 m=mesh();box(m,(0,.28,0),(.40,.31,.095),'piedra_oscura')
 box(m,(0,.28,.056),(.29,.22,.037),'oro_oscuro')
 cylinder(m,(0,.20,.082),.08,.10,'COLOR_DINAMICO',10)
 return m

def templo():
 m=mesh()
 box(m,(0,-1.20,0),(54,.39,46),'muro_fondo')
 box(m,(0,5,-22),(54,12,1.2),'piedra_oscura')
 for x in range(-26,27,4):
  box(m,(x,5.2,-21.29),(.07,11.3,.025),'fisura')
 for y in [-.7,2.2,5.1,8.0]:
  box(m,(0,y,-21.27),(53.2,.10,.06),'piedra')
 box(m,(0,8.42,-21.28),(53.2,.51,.32),'oro_oscuro')
 box(m,(0,8.67,-21.25),(53.2,.055,.36),'oro')
 for x in range(-24,25,4):
  box(m,(x,6.55,-21.08),(1.23,.30,.09),'oro')
  box(m,(x,5.94,-21.06),(.28,1.1,.09),'jade')
  ellipsoid(m,(x,5.93,-20.98),(.09,.10,.04),'jade_claro',8,4)
 for z in [-19.8,-14.4,-9,0,9,17]:
  for x in [-26.3,26.3]:
   cylinder(m,(x,-.98,z),.24,.06,'oro_oscuro',10)
 return m

def columna():
 m=mesh()
 cylinder(m,(0,-.20,0),1.50,.55,'piedra_oscura',12,r_top=1.30)
 cylinder(m,(0,.35,0),1.12,.45,'piedra_clara',12,r_top=1.01)
 cylinder(m,(0,.80,0),.89,8.55,'piedra',12,r_top=.87)
 for i in range(12):
  a=TAU*i/12
  beam(m,(.89*math.cos(a),.83,.89*math.sin(a)),(.87*math.cos(a),9.33,.87*math.sin(a)),.026,'piedra_clara',6)
 for y in [1.0,4.6,8.6]:
  torus(m,(0,y,0),.96,.075,'jade_oscuro',24,6)
  torus(m,(0,y+.09,0),.97,.029,'oro',24,5)
 cylinder(m,(0,9.35,0),1.12,.42,'piedra_clara',12)
 cylinder(m,(0,9.77,0),1.40,.48,'piedra_oscura',12,r_top=1.48)
 for x in [-1.03,1.03]:
  for z in [-1.03,1.03]:
   box(m,(x,9.89,z),(.20,.11,.20),'oro')
 return m

def antorcha():
 m=mesh()
 # Base local a Y=0; llamar en mundo (x,2.4,-21).
 cylinder(m,(0,0,0),.19,.08,'oro_oscuro',10)
 cylinder(m,(0,.08,0),.13,1.43,'piedra_oscura',10,r_top=.11)
 torus(m,(0,1.44,0),.17,.045,'oro',16,5)
 cylinder(m,(0,1.53,0),.27,.12,'oro_oscuro',12,r_top=.30)
 cylinder(m,(0,1.62,0),.23,.09,'sombra',12)
 for i in range(4):
  a=TAU*i/4
  beam(m,(.16*math.cos(a),1.56,.16*math.sin(a)),(.12*math.cos(a),1.10,.12*math.sin(a)),.022,'oro',6)
 return m

def llama():
 m=mesh()
 # Pivot de animacion a altura de la llama, y=0 en x,y,z del juego.
 ellipsoid(m,(0,-.02,0),(.32,.36,.30),'brasa',12,7)
 ellipsoid(m,(0,.05,0),(.20,.27,.19),'fuego',12,7)
 ellipsoid(m,(0,.28,0),(.10,.28,.10),'fuego',10,6)
 return m

def main():
 specs=[
 ('losa_marco',losa(),'centro de tablero, plano de juego Y=0','Colocar en centrosTableros; dentro de rlTranslate/rlRotate. Mide 12x12.'),
 ('banda_jugador',banda_jugador(),'centro del frente, Y=-.3','Colocar (0,-.30,6.02) relativo a losa; COLOR_DINAMICO tintado por jugador.'),
 ('muro_bloque',muro(),'suelo central de celda','Por cada celda #, posicion (c-5,r-5) en X/Z; limites de planta .98 y altura .61.'),
 ('glifo_muro',glifo(),'centro superior de muro','Colocar sobre muro en Y=.61 si (c*7+r*3)%5==0.'),
 ('agujero',agujero(),'centro de celda en suelo','Colocar por celda o; deteccion logica de caida radio .34, solo visual.'),
 ('marca_salida',salida(),'centro de celda en suelo','Colocar en (inicioColumna-5,inicioFila-5), Y=.05.'),
 ('checkpoint',control(),'base del disco','Colocar en centro de checkpoint, Y=.05; tintar COLOR_DINAMICO oro/jade al activarse.'),
 ('altar',altar(),'base del pedestal','Centro de celda M; solo visual.'),
 ('esfera_jade',esfera(),'centro de esfera','PosicionLocal(e.x,e.z,.28*escala+.03); escalar por caida.'),
 ('flecha_trampa',trampa(),'centro de baldosa','Orientar hacia direccion de dardos; tintar aviso rojo/naranja.'),
 ('boquilla_trampa',boquilla(),'centro del soporte','Instanciar en extremo de pasillo y rotar segun eje, sin colision.'),
 ('templo_fondo',templo(),'centro de escena global','Mundo (0,0,0), fuera de tableros; suelo Y=-1.'),
 ('columna_templo',columna(),'base al suelo del templo','Mundo X+-23, Z=-14 y +12; modelo llega hasta Y=10.25.'),
 ('antorcha',antorcha(),'base del soporte','Mundo X=-15,-9,-3,3,9,15; Y=2.4, Z=-21.'),
 ('llama',llama(),'centro de llama','Mundo en (x,4.3+parpadeo*.08,-20.9); escala por parpadeo.'),
 ]
 for name,model,pivot,usage in specs:save(name,model,pivot,usage)
 (glb.ROOT/'manifest.json').write_text(json.dumps({
  'version':1,'rama_referencia':'claude/expansion-party','commit_referencia':'ea5472c',
  'unidades':'1 unidad GLB = 1 unidad raylib','ejes':'Y arriba; tablero: celda (c,r) en X=c-5, Z=r-5',
  'licencia':'Arte original procedural sin recursos externos',
  'nota':'La simulacion de esfera, muros y trampas permanece en el codigo del minijuego.',
  'modelos':glb.MANIFEST},ensure_ascii=False,indent=2),encoding='utf8')
 print(len(glb.MANIFEST),'GLB;',sum(x['triangulos'] for x in glb.MANIFEST),'triangulos')

if __name__=='__main__':main()
