"""Genera GLB 2.0 originales para Cajas del Puerto; Python 3 sin paquetes externos."""
from geometria_glb import *
import geometria_glb as glb

# Colores de terminal portuaria nocturna: azul marino, grua mostaza, madera,
# cobre y ocho contenedores cuyo color se decide durante la partida.
glb.PALETTE.update({
 'madera':'#76503a','madera_clara':'#a4774e','madera_oscura':'#40382f',
 'metal':'#9dafbf','metal_oscuro':'#354655','negro':'#121924',
 'azul':'#28607b','azul_oscuro':'#15364c','turquesa':'#4ba5af',
 'oro':'#e3ad3e','oro_oscuro':'#a47632','cereza':'#b0493f',
 'cereza_oscura':'#743a39','crema':'#efe8c9','blanco':'#eaf1ed',
 'violeta':'#604963','violeta_oscuro':'#403949','rosa':'#d08876',
 'COLOR_DINAMICO':'#ffffff','BOMBILLAS':'#fff1b6',
 'agua':'#13384d','cristal':'#83c4d1','oxido':'#945a37',
})


def muelle():
 m=mesh()
 # Origen en el centro del piso jugable. Plataforma y=0; z de -7 a +7.
 box(m,(0,-.31,0),(22,.62,14),'madera_oscura')
 for i in range(35):
  z=-6.8+i*.4
  box(m,(0,.018,z),(21.88,.036,.35),['madera','madera_clara','madera_oscura'][i%3])
  for j in [-1,1]:
   for x in [-10.55,-6.3,-2.05,2.2,6.45,10.7]:
    cylinder(m,(x,.044,z+j*.11),.014,.004,'metal_oscuro',6)
 for x in [-10.7,10.7]:
  beam(m,(x,-.62,-7),(x,-.62,7),.09,'metal_oscuro',6)
 for z in [-6.2,-2.4,1.4,5.2]:
  for x in [-10.55,10.55]:
   box(m,(x,-.94,z),(.48,1.25,.5),'madera_oscura')
   box(m,(x,-1.55,z),(.74,.16,.74),'oro_oscuro')
 for z in [-6.7,6.7]:
  beam(m,(-10.9,-.57,z),(10.9,-.57,z),.11,'metal_oscuro',6)
 # Liston en borde delantero, lejos del maximo z=5.5 de jugadores.
 for x in [-10.55,10.55]:
  for z in [-6.3,-2.8,.7,4.2,6.8]:
   box(m,(x,.14,z),(.15,.28,.15),'oro')
 return m


def portico():
 m=mesh()
 for x in [-10.3,10.3]:
  for z in [0,-1.2]:
   box(m,(x,2.55,z),(.5,5.1,.5),'oro')
   box(m,(x,.13,z),(.83,.26,.76),'metal_oscuro')
   box(m,(x,4.86,z),(.68,.14,.68),'oro_oscuro')
  beam(m,(x,1.65,0),(x,3.9,-1.2),.065,'metal_oscuro',8)
  beam(m,(x,1.65,-1.2),(x,3.9,0),.065,'metal_oscuro',8)
  beam(m,(x,5.18,-1.35),(x,5.18,.15),.14,'oro_oscuro',8)
 # Viga viajera; altura total 5.6, cabina y carro se montan aparte.
 box(m,(0,5.35,0),(21,.5,.8),'oro')
 for z in [-.43,.43]:
  box(m,(0,5.67,z),(20.7,.11,.09),'metal_oscuro')
  box(m,(0,5.02,z),(20.7,.10,.10),'oro_oscuro')
 for x in range(-10,11,2):
  for z in [-.49,.49]:
   box(m,(x,5.1,z),(.25,.11,.025),'negro')
 # Pasarela tecnica posterior, fuera de la posicion del jugador solitario.
 box(m,(0,5.08,-1.05),(19.8,.11,.65),'metal_oscuro')
 for x in [-9.55,9.55]:
  for z in [-1.4,-.75]:
   box(m,(x,5.55,z),(.045,.98,.045),'oro_oscuro')
  beam(m,(x,6.04,-1.4),(x,6.04,-.75),.038,'oro_oscuro',6)
 # Escalera exterior del soporte derecho, libre de la zona de contenedores.
 for i in range(14):
  y=.45+i*.33
  beam(m,(10.56,y,-.53),(11.15,y,-.53),.035,'metal',6)
 for x in [10.56,11.15]:beam(m,(x,.37,-.53),(x,5.05,-.53),.04,'metal',6)
 return m


def carro():
 m=mesh();box(m,(0,0,0),(2.4,.18,1.8),'metal_oscuro')
 box(m,(0,.17,0),(2.18,.16,1.62),'oro_oscuro')
 # Plataforma para el jugador: pies aproximadamente a y=.1 sobre y=5.7.
 box(m,(0,.275,0),(1.86,.07,1.33),'metal')
 for x in [-.87,.87]:
  for z in [-.84,.84]:
   cylinder(m,(x,-.19,z),.18,.14,'negro',12)
   cylinder(m,(x,-.19,z),.09,.145,'oro',8)
 for z in [-.73,.73]:
  box(m,(0,.57,z),(2.25,.56,.07),'oro')
  box(m,(0,.95,z),(2.22,.07,.075),'metal')
 # Abertura hacia la carga: frontal +Z sin baranda central.
 for x in [-1.08,1.08]:
  beam(m,(x,.24,.64),(x,.93,.64),.045,'oro',6)
 return m


def cabina():
 m=mesh()
 # Conjunto de bastidores que viaja con el carro. Interior a la vista.
 for x in [-1.1,1.1]:
  for z in [-.82,.82]:
   beam(m,(x,.32,z),(x,2.62,z),.052,'oro',8)
 for y in [1.16,2.62]:
  for z in [-.82,.82]:
   beam(m,(-1.1,y,z),(1.1,y,z),.05,'oro',8)
  for x in [-1.1,1.1]:beam(m,(x,y,-.82),(x,y,.82),.05,'oro',8)
 box(m,(0,2.75,0),(2.58,.22,2.05),'oro')
 for i in range(7):box(m,(-1.04+i*.35,2.875,0),(.06,.03,1.95),'oro_oscuro')
 # Panel trasero de mando; pantalla en +Z, visible desde la camara del juego.
 box(m,(0,1.27,-.76),(1.15,.49,.11),'metal_oscuro')
 box(m,(0,1.27,-.69),(.71,.24,.022),'turquesa')
 for x in [-.72,-.48,.48,.72]:
  ellipsoid(m,(x,1.08,-.66),(.04,.04,.025),'oro',8,4)
 for x in [-1.18,1.18]:
  for z in [-.87,.87]:
   box(m,(x,.40,z),(.18,.18,.18),'metal_oscuro')
 return m


def brazo():
 m=mesh()
 # Pivote (ganchoX,5.55,-5); punta frontal (0,0,+2.5).
 box(m,(0,0,1.3),(.28,.20,2.6),'oro')
 beam(m,(-.17,-.07,.05),(-.17,-.07,2.5),.025,'metal_oscuro',6)
 beam(m,(.17,-.07,.05),(.17,-.07,2.5),.025,'metal_oscuro',6)
 box(m,(0,-.02,2.49),(.53,.28,.5),'metal_oscuro')
 cylinder(m,(0,-.23,2.5),.12,.08,'oro',12)
 return m


def cable():
 m=mesh();beam(m,(0,0,0),(0,-1,0),.04,'metal',8)
 for y in [-.10,-.91]:cylinder(m,(0,y,0),.07,.05,'metal_oscuro',8)
 return m


def gancho():
 m=mesh()
 # Pivote superior en yGancho. El pulpo de carga acaba menos de 0.85 abajo.
 box(m,(0,-.13,0),(.38,.25,.40),'metal_oscuro')
 cylinder(m,(0,-.31,0),.18,.08,'oro',12)
 beam(m,(0,-.31,0),(0,-.49,0),.08,'metal',10)
 points=[(0,-.48,0),(.03,-.65,0),(.13,-.78,0),(.28,-.75,0),(.35,-.64,0),(.31,-.59,0)]
 for a,b in zip(points,points[1:]):beam(m,a,b,.066,'metal',9,r_end=.05)
 ellipsoid(m,(.31,-.59,0),(.06,.04,.06),'oro',10,5)
 return m


def contenedor():
 m=mesh()
 # Hueco real. Unidades y=0..2.2, -x+.95, -z+1.00.
 box(m,(0,-.027,0),(1.9,.054,2.0),'metal_oscuro')
 for x in [-.93,.93]:box(m,(x,1.1,0),(.045,2.2,1.99),'COLOR_DINAMICO')
 box(m,(0,1.1,-.98),(1.90,2.20,.04),'COLOR_DINAMICO')
 box(m,(0,2.21,0),(1.96,.09,2.06),'COLOR_DINAMICO')
 for x in [-.82,-.58,-.34,-.10,.14,.38,.62,.86]:
  for xx in [-1,1]:
   # Perfil de chapa exterior a 0.98, dentro del limite 1.96 del techo.
   box(m,(xx*.959,1.08,x),( .028,1.91,.055),'COLOR_DINAMICO')
 for x in [-.87,.87]:
  for z in [-.94,.94]:
   box(m,(x,1.1,z),(.10,2.23,.12),'metal_oscuro')
   box(m,(x,2.27,z),(.13,.08,.13),'oro_oscuro')
   box(m,(x,-.04,z),(.13,.10,.13),'oro_oscuro')
 for z in [-.8,-.58,-.36,-.14,.08,.30,.52,.74]:
  box(m,(0,2.277,z),(1.8,.018,.04),'metal_oscuro')
 # Cara frontal +Z: hueco visual, marco y pivotes de puertas reutilizables.
 for x in [-.88,.88]:box(m,(x,1.09,1.005),(.07,2.14,.07),'metal_oscuro')
 for y in [.07,2.15]:box(m,(0,y,1.005),(1.83,.07,.07),'metal_oscuro')
 return m


def puerta(lado):
 m=mesh();x= .425*lado
 box(m,(x,0,0),(.85,2.0,.07),'COLOR_DINAMICO')
 for dx in [-.38,-.16,.08,.30]:
  box(m,(x+dx*lado,0,.047),(.036,1.85,.028),'metal_oscuro')
 for y in [-.85,.85]:box(m,(x,y,.055),(.80,.045,.025),'metal_oscuro')
 for y in [-.66,.66]:
  cylinder(m,(.03*lado,y,.073),.045,.012,'oro',8)
  box(m,(.08*lado,y,.09),(.11,.035,.025),'oro')
 for y in [-.75,.75]:
  ellipsoid(m,(.80*lado,y,.045),(.038,.065,.045),'oro_oscuro',8,4)
 return m


def cerrado_decoracion():
 m=contenedor()
 # Origen en el suelo, profundidad decorativa 4.0: transformar Z de cada vertice.
 long=mesh()
 for mat,ts in m.items():
  for tr in ts:long[mat].append(tuple((v[0],v[1],v[2]*2) for v in tr))
 for side in [-1,1]:
  door=puerta(side)
  for mat,ts in transform(door,p=(side*.85,1.1,2.09)).items():long[mat].extend(ts)
 return long


def farol():
 m=mesh();cylinder(m,(0,0,0),.17,.20,'metal_oscuro',12,r_top=.13)
 cylinder(m,(0,.16,0),.095,2.78,'metal_oscuro',10,r_top=.075)
 beam(m,(0,2.65,0),(.3,3.18,0),.055,'metal',8)
 cylinder(m,(0,2.90,0),.22,.10,'oro_oscuro',10)
 for x in [-.18,.18]:
  for z in [-.18,.18]:beam(m,(x,3.07,z),(x,3.52,z),.027,'metal',5)
 box(m,(0,3.12,0),(.38,.06,.38),'oro_oscuro')
 ellipsoid(m,(0,3.3,0),(.19,.22,.19),'BOMBILLAS',12,6)
 cylinder(m,(0,3.51,0),.26,.12,'metal_oscuro',10,r_top=.06)
 ellipsoid(m,(0,3.66,0),(.07,.07,.07),'oro',8,4)
 return m


def bolardo():
 m=mesh();cylinder(m,(0,0,0),.29,.08,'madera_oscura',12)
 cylinder(m,(0,.08,0),.18,.7,'metal_oscuro',10)
 cylinder(m,(0,.78,0),.27,.14,'metal_oscuro',10)
 torus(m,(0,.63,0),.2,.06,'oxido',18,6)
 torus(m,(0,.53,0),.2,.06,'oxido',18,6)
 beam(m,(-.33,.55,0),(.33,.55,0),.065,'metal',8)
 return m


def ancla():
 m=mesh();beam(m,(0,0,0),(0,.75,0),.065,'oro',10)
 torus(m,(0,.78,0),.15,.052,'oro',16,6,plane='xy')
 beam(m,(-.30,.52,0),(.30,.52,0),.065,'oro',8)
 beam(m,(-.34,.50,0),(-.34,.62,0),.055,'oro',8)
 beam(m,(.34,.50,0),(.34,.62,0),.055,'oro',8)
 beam(m,(0,.06,0),(-.43,.16,0),.067,'oro',10,r_end=.04)
 beam(m,(0,.06,0),(.43,.16,0),.067,'oro',10,r_end=.04)
 beam(m,(-.43,.16,0),(-.5,.31,0),.04,'oro',8)
 beam(m,(.43,.16,0),(.5,.31,0),.04,'oro',8)
 for x in [-.5,.5]:ellipsoid(m,(x,.31,0),(.055,.075,.07),'oro',8,4)
 return m


def barco():
 m=mesh()
 # Origen de modelo en el plano de agua del juego (Y=-0.9).
 top=[(-17.25,0),(-13.8,-3.0),(13.8,-3.0),(17.25,0),(13.8,3.0),(-13.8,3.0)]
 low=[(-15.7,0),(-13.0,-2.45),(13.0,-2.45),(15.7,0),(13.0,2.45),(-13.0,2.45)]
 for i in range(6):
  j=(i+1)%6
  a=top[i];b=top[j];la=low[i];lb=low[j]
  quad(m,(a[0],3.0,a[1]),(la[0],-.4,la[1]),(lb[0],-.4,lb[1]),(b[0],3,b[1]),'cereza_oscura')
  tri(m,(0,-.4,0),(lb[0],-.4,lb[1]),(la[0],-.4,la[1]),'cereza_oscura')
  tri(m,(0,3,0),(a[0],3,a[1]),(b[0],3,b[1]),'madera_oscura')
  beam(m,(a[0],3,a[1]),(b[0],3,b[1]),.10,'oro_oscuro',8)
 for x in [-13,-9,-5,-1,3,7,11]:
  box(m,(x,3.03,0),(2.0,.08,5.15),'madera' if int(x)%3 else 'madera_clara')
 for x in [-12,-8,-4,0,4]:
  box(m,(x,4.12,0),(2.7,2.1,2.1),'metal_oscuro')
  for z in [-1.04,1.04]:
   for xx in [-1.06,0,1.06]:box(m,(x+xx,4.14,z),(.045,1.93,.05),'COLOR_DINAMICO')
 # Puente de mando sobre popa: en juego aparece a x=9.
 box(m,(9,4.50,0),(6,3.0,4.2),'metal')
 for x in [6.7,8.05,9.4,10.75]:
  box(m,(x,5.0,2.15),(1.0,.68,.045),'cristal')
  box(m,(x,5.0,-2.15),(1.0,.68,.045),'cristal')
 box(m,(9,6.1,0),(6.45,.18,4.5),'metal_oscuro')
 box(m,(10.5,7.16,0),(1.3,2.3,1.4),'cereza')
 box(m,(10.5,8.43,0),(1.45,.20,1.55),'negro')
 for x in [-14,-7,0,7,14]:
  for z in [-2.75,2.75]:
   beam(m,(x,3.05,z),(x,3.95,z),.035,'metal',6)
  beam(m,(x,3.88,-2.75),(x,3.88,2.75),.035,'metal',6)
 for x in [-10,-6,-2,2,6,10]:
  for z in [-2.98,2.98]:
   ellipsoid(m,(x,2.27,z),(.105,.105,.105),'BOMBILLAS',8,5)
 for x in [6.4,7.6,8.8,10]:
  box(m,(x,5.05,2.19),(.65,.30,.04),'cristal')
 return m


def marca():
 m=mesh();cylinder(m,(0,0,0),.56,.045,'negro',20)
 torus(m,(0,.048,0),.53,.034,'cereza',20,5)
 for i in range(6):
  a=TAU*i/6
  beam(m,(.26*math.cos(a),.06,.26*math.sin(a)),(.43*math.cos(a+.36),.08,.43*math.sin(a+.36)),.012,'oro',5)
 return m


def main():
 specs=[
 ('muelle',muelle(),'centro de superficie; Y=0','mundo (0,0,0.75); borde Z=-6.25 a 7.75.'),
 ('grua_portico',portico(),'origen del portico en el muelle','mundo (0,0,-5), columnas X+-10.3 y altura de traviesa 5.35.'),
 ('grua_carro',carro(),'centro del carro a altura de traviesa','mundo (ganchoX,5.7,-5); piso bajo jugador del equipo solo.'),
 ('grua_cabina',cabina(),'mismo origen que carro','mundo (ganchoX,5.7,-5); arco abierto para visualizar jugador.'),
 ('grua_brazo',brazo(),'union con el carro','mundo (ganchoX,5.55,-5); punta en Z=-2.5.'),
 ('grua_cable_unidad',cable(),'inicio superior, cable hacia Y negativo','instanciar en (ganchoX,5.55,-2.5), escalar SOLO Y hasta 5.55-yGancho.'),
 ('grua_gancho',gancho(),'punto superior del gancho','mundo (ganchoX,yGancho,-2.5); yGancho=4.2+bob.'),
 ('contenedor_cuerpo',contenedor(),'centro XZ, suelo del contenedor Y=0','mundo (2.1*(indice-3.5),0,-2.5). Interior abierto.'),
 ('contenedor_puerta_izquierda',puerta(-1),'bisagra izquierda en X=-0.85, Y del centro','puerta izquierda: (-.85,1.1,1.08) relativo a contenedor_cuerpo.'),
 ('contenedor_puerta_derecha',puerta(1),'bisagra derecha en X=+0.85, Y del centro','puerta derecha: (+.85,1.1,1.08). Apertura logica 0..1.'),
 ('contenedor_decoracion',cerrado_decoracion(),'centro XZ, suelo de contenedor','cajas de fondo: (X+/-12.5,0,-5) y escalon Y+2.2.'),
 ('farol',farol(),'base a nivel de muelle','(-10.5|10.5,0,1.0|6.5). Luz en Y=3.3.'),
 ('bolardo',bolardo(),'base a nivel de muelle','X=-10,-6,-2,2,6,10; Z=7.8.'),
 ('ancla_dorada',ancla(),'ancla vertical a nivel del techo','refuerzo si contenedor.reforzado; centro del techo Y=2.26.'),
 ('barco_fondo',barco(),'plano de agua bajo cubierta','mundo (0,-0.9,-17). 34.5 x 6.2.'),
 ('marca_golpe',marca(),'centro de disco sobre el techo','solo si contenedor.marcas>0; colocar sobre Y=2.32.'),
 ]
 for name,model,pivot,usage in specs:save(name,model,pivot,usage)
 (ROOT/'manifest.json').write_text(json.dumps({'version':1,'rama_referencia':'claude/expansion-party','commit_referencia':'ea5472c',
  'unidades':'1 unidad = 1 unidad raylib','ejes':'Y arriba; vista frontal +Z',
  'licencia':'Arte original sin recursos externos','modelos':MANIFEST},ensure_ascii=False,indent=2),encoding='utf8')
 print(f'{len(MANIFEST)} GLB; {sum(x["triangulos"] for x in MANIFEST)} triangulos; {sum(x["bytes"] for x in MANIFEST)} bytes')

if __name__=='__main__':main()
