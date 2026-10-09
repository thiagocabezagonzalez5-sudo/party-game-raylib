"""Modelos procedurales modulares para Grúa de Chatarra; Python estándar."""
from geometria_glb import *
import geometria_glb as glb

glb.PALETTE.update({
 'suelo':'#413a36','pozo':'#58463c','borde':'#9b7645','metal':'#7f8995',
 'metal_luz':'#adb7bc','metal_oscuro':'#333b47','sombra':'#242932',
 'oxido':'#a25d3d','oxido_luz':'#d1834b','oxido_sombra':'#704432',
 'amarillo':'#f2c74a','amarillo_luz':'#ffe47e','franja':'#333641',
 'rojo':'#d55a4d','azul':'#5484bd','crema':'#dbc16c',
 'pantalla':'#7de8d4','luz':'#fff0b1','goma':'#202632',
 'cobre':'#c8895b','peligro':'#cc6851','interior':'#151d27',
 'COLOR_DINAMICO':'#ffffff','BOMBILLAS':'#fff4cc'})

def floor():
 m=mesh();box(m,(0,-.25,.2),(21,.5,15),'suelo');box(m,(0,-.18,.2),(18.2,.4,11.2),'pozo')
 for x in (-9.1,9.1):box(m,(x,.08,.2),(.13,.17,11.5),'borde')
 for z in (-5.4,5.8):box(m,(0,.08,z),(18.3,.17,.13),'borde')
 for i in range(36):
  x=-8.5+(i*37%170)/10;z=-4.9+(i*53%104)/10
  box(m,(x,.045,z),(.25+.08*(i%3),.08,.13),'oxido_sombra' if i%3 else 'metal')
 return m

def debris():
 m=mesh()
 for i in range(14):
  x=-1.2+(i*17%27)/10;z=-.8+(i*11%19)/10
  box(m,(x,.10+.035*(i%3),z),(.38,.16,.18),'oxido' if i%2 else 'metal')
 return m

def wall():
 m=mesh();box(m,(0,3,0),(24,6,.8),'metal_oscuro')
 for i in range(13):
  x=-11.5+i*1.9;box(m,(x,3,.43),(.07,5.8,.07),'metal')
 for y in (.55,5.5):box(m,(0,y,.44),(24,.10,.08),'oxido_sombra')
 for i in range(11):
  x=-11+i*2.2;box(m,(x,.75,.48),(.65,.13,.035),'amarillo' if i%2 else 'franja')
 return m

def gear(radius=2,teeth=12):
 m=mesh();beam(m,(0,0,-.27),(0,0,.25),radius*.81,'oxido',20)
 torus(m,(0,0,.28),radius*.7,.095,'oxido_luz',24,5,plane='xy')
 for i in range(teeth):
  a=TAU*i/teeth;x=radius*math.cos(a);y=radius*math.sin(a)
  box(m,(x,y,0),(.45,.45,.55),'oxido_luz' if i%3==0 else 'oxido')
 beam(m,(0,0,-.35),(0,0,.42),radius*.27,'metal_oscuro',12)
 for i in range(6):
  a=TAU*i/6;beam(m,(.35*math.cos(a),.35*math.sin(a),.41),(.65*radius*math.cos(a),.65*radius*math.sin(a),.41),.065,'metal_luz',6)
 return m

def press_frame():
 m=mesh();box(m,(0,.2,0),(2.4,.4,1.8),'metal_oscuro')
 for x in (-.86,.86):
  box(m,(x,2.62,0),(.33,5.1,.5),'metal')
  box(m,(x,2.6,.30),(.10,4.6,.06),'metal_luz')
 box(m,(0,5.1,0),(2.4,.6,1.7),'metal_oscuro')
 for z in (-.58,.58):beam(m,(0,4.72,z),(0,2.7,z),.14,'metal_luz',10)
 box(m,(0,.55,.91),(1.5,.38,.10),'peligro')
 return m

def press_plate():
 m=mesh();box(m,(0,0,0),(2,.5,1.4),'amarillo')
 box(m,(0,-.26,0),(2.1,.08,1.48),'metal_oscuro')
 for x in (-.78,.78):
  for z in (-.53,.53):cylinder(m,(x,.26,z),.08,.06,'metal_luz',8)
 for i in range(5):box(m,(-.75+i*.38,.255,.58),(.15,.02,.025),'franja')
 return m

def car(color):
 m=mesh();box(m,(0,.26,0),(1.6,.45,3),color)
 box(m,(0,.51,0),(1.16,.22,1.55),'metal_oscuro')
 for x in (-.82,.82):
  for z in (-.9,.9):ellipsoid(m,(x,.24,z),(.14,.21,.29),'goma',10,5)
 for z in (-1.5,1.5):box(m,(0,.28,z),(1.55,.12,.09),'metal_luz')
 for x in (-.52,.52):box(m,(x,.39,1.55),(.19,.09,.025),'amarillo_luz')
 return m

def conveyor():
 m=mesh();box(m,(0,.20,0),(1.2,.4,8),'metal_oscuro')
 box(m,(0,.405,0),(1.03,.035,7.7),'goma')
 for x in (-.56,.56):box(m,(x,.51,0),(.08,.24,8),'metal')
 for i in range(8):box(m,(0,.439,-3.5+i),(1.02,.035,.18),'amarillo')
 for z in (-3.55,3.55):
  beam(m,(-.56,.31,z),(.56,.31,z),.23,'metal_luz',12)
 for z in (-2.75,2.75):
  for x in (-.45,.45):box(m,(x,-.12,z),(.16,.55,.16),'metal')
 return m

def scrap():
 m=mesh();box(m,(0,.27,0),(1.7,.45,1.18),'oxido_sombra')
 for i in range(8):
  x=-.65+(i*7%13)/10;z=-.45+(i*11%9)/10;y=.52+(i%3)*.18
  box(m,(x,y,z),(.50,.16,.24),['oxido','metal','oxido_luz'][i%3])
 beam(m,(-.65,.62,-.36),(.75,1.06,.42),.055,'metal_luz',6)
 cylinder(m,(-.34,.5,.45),.23,.10,'metal_oscuro',10)
 return m

def lamp():
 m=mesh();cylinder(m,(0,0,0),.24,.22,'metal_oscuro',10)
 beam(m,(0,.15,0),(0,5.95,0),.10,'metal',8)
 beam(m,(0,5.84,0),(0,5.84,-.42),.065,'metal_luz',6)
 cylinder(m,(0,5.74,-.45),.36,.24,'metal_oscuro',12,r_top=.25)
 ellipsoid(m,(0,5.78,-.45),(.25,.14,.25),'BOMBILLAS',12,5)
 torus(m,(0,5.74,-.45),.32,.035,'amarillo',16,4)
 return m

def hopper():
 m=mesh();box(m,(0,.55,0),(2.4,1.1,1.6),'metal_oscuro')
 box(m,(0,.22,-1.5),(2,.44,1.55),'metal')
 box(m,(0,.66,-2.2),(1.6,.88,.18),'metal_oscuro')
 # Embudo de sección rectangular, abierto y con interior oscuro.
 y0,y1=1.10,1.71
 lower=[(-.52,y0,-.28),(.52,y0,-.28),(.52,y0,.28),(-.52,y0,.28)]
 upper=[(-1.12,y1,-.74),(1.12,y1,-.74),(1.12,y1,.74),(-1.12,y1,.74)]
 for i in range(4):
  j=(i+1)%4;quad(m,lower[i],lower[j],upper[j],upper[i],'metal')
 box(m,(0,1.11,0),(.98,.03,.5),'interior')
 for x in (-1.13,1.13):box(m,(x,y1,0),(.09,.08,1.55),'COLOR_DINAMICO')
 for z in (-.75,.75):box(m,(0,y1,z),(2.35,.08,.09),'COLOR_DINAMICO')
 box(m,(0,.81,.81),(2.24,.12,.06),'COLOR_DINAMICO')
 for x in (-.87,.87):box(m,(x,.55,.83),(.12,.32,.06),'amarillo')
 return m

def claw():
 m=mesh();cylinder(m,(0,-.15,0),.45,.30,'COLOR_DINAMICO',14,r_top=.5)
 cylinder(m,(0,.15,0),.22,.22,'metal_oscuro',10)
 torus(m,(0,-.02,0),.44,.05,'metal_luz',16,5)
 for i in range(4):
  a=TAU*(i+.5)/4
  box(m,(.3*math.cos(a),-.13,.3*math.sin(a)),(.13,.13,.13),'metal_oscuro')
 return m

def tine():
 m=mesh();box(m,(0,-.18,0),(.11,.47,.11),'metal_luz')
 box(m,(0,-.42,-.09),(.11,.13,.25),'metal')
 return m

def trolley():
 m=mesh();box(m,(0,0,0),(.7,.3,.7),'metal_oscuro')
 box(m,(0,-.17,0),(.22,.10,.22),'metal_luz')
 for x in (-.27,.27):
  for z in (-.28,.28):ellipsoid(m,(x,.15,z),(.10,.10,.10),'goma',8,4)
 return m

def marker():
 m=mesh();torus(m,(0,.045,0),.55,.027,'COLOR_DINAMICO',32,4)
 cylinder(m,(0,.045,0),.08,.012,'BOMBILLAS',12)
 return m

def nut():
 m=mesh();cylinder(m,(0,0,0),.38,.22,'metal_luz',6)
 cylinder(m,(0,.22,0),.16,.018,'interior',12)
 torus(m,(0,.242,0),.17,.025,'metal',16,4)
 return m

def smallgear():
 m=mesh();cylinder(m,(0,0,0),.39,.2,'metal',12)
 for i in range(10):
  a=TAU*i/10;box(m,(.48*math.cos(a),.1,.48*math.sin(a)),(.17,.20,.17),'metal_luz')
 cylinder(m,(0,.205,0),.15,.018,'interior',12)
 return m

def motor():
 m=mesh();box(m,(0,.34,0),(.95,.68,.7),'metal_oscuro')
 for x in (-.35,.35):box(m,(x,.36,.36),(.17,.52,.04),'metal_luz')
 cylinder(m,(0,.68,0),.26,.23,'cobre',12)
 cylinder(m,(0,.91,0),.12,.11,'metal_luz',10)
 box(m,(0,.14,.43),(.55,.12,.18),'oxido')
 return m

def battery():
 m=mesh();box(m,(0,.35,0),(.60,.70,.45),'amarillo')
 box(m,(0,.52,.235),(.48,.12,.018),'franja')
 for x in (-.16,.16):
  cylinder(m,(x,.70,0),.09,.13,'metal_luz',10)
 return m

def cartridge():
 m=mesh();cylinder(m,(0,0,0),.32,.55,'peligro',12,r_top=.28)
 cylinder(m,(0,.55,0),.27,.09,'amarillo',12)
 torus(m,(0,.4,0),.295,.026,'franja',16,4)
 for a in (-.12,.12):box(m,(a,.26,.31),(.08,.27,.025),'amarillo')
 return m

def main():
 specs=[
 ('suelo_desguace',floor(),'origen global','Centro de arena: X=0, Z=0; patio exterior 21×15.'),
 ('escombros',debris(),'pie central','Decoración baja del pozo; dispersar copias sin tapar objetivos.'),
 ('muro_fondo',wall(),'centro al pie','Situar centro en (0,0,-9.2).'),
 ('engranaje_gigante',gear(),'centro geométrico','Dos instancias en (-7,3.4,-8.2), (7.5,3.4,-8.2). Rotar sobre eje Z para animación.'),
 ('prensa_estructura',press_frame(),'pie central','Centro en (10.6,0,-3.5).'),
 ('prensa_plato',press_plate(),'centro geométrico','Posición (10.6,4-1.5*prensa,-3.5), prensa=0.5+0.5*sin(t*1.4).'),
 ('auto_aplastado_rojo',car('rojo'),'pie central','Apilar con base Y=0.05, separación vertical 0.5.'),
 ('auto_aplastado_azul',car('azul'),'pie central','Apilar con base Y=0.55, separación vertical 0.5.'),
 ('auto_aplastado_amarillo',car('crema'),'pie central','Apilar con base Y=1.05, separación vertical 0.5.'),
 ('cinta_transportadora',conveyor(),'centro de su planta','Dos instancias X=±9.9, Z=1.6; animar franjas por desplazamiento de material si se desea.'),
 ('pila_chatarra',scrap(),'pie central','Nueve instancias X=-10+2.5*i, Z=6.6; escala Y entre .8 y 1.8.'),
 ('foco_industrial',lamp(),'pie central','Cuatro instancias X=-8+5.4*i, Z=8.'),
 ('tolva_equipo',hopper(),'pie central','X=(puesto-(N-1)/2)*4.6, Z=-6.3; tintar COLOR_DINAMICO.'),
 ('garra_iman',claw(),'centro del imán','Centro en (estado.x,altura,estado.z), tinte del jugador.'),
 ('garra_pinza',tine(),'unión superior','Cuatro instancias en ángulo π/4+kπ/2, radio=apertura*0.7; altura -0.02 respecto al imán.'),
 ('carro_grua',trolley(),'centro','Centro (estado.x,8,estado.z); cable dinámico entre carro e imán.'),
 ('marca_garra',marker(),'centro','Posición de estado (x,0,z), tintar por jugador.'),
 ('objeto_tuerca',nut(),'pie central','Posición del objeto, valor +1; radio lógico 1.1.'),
 ('objeto_engranaje',smallgear(),'pie central','Posición del objeto, valor +3; radio lógico .9.'),
 ('objeto_motor',motor(),'pie central','Posición del objeto, valor +5; radio lógico .7.'),
 ('objeto_bateria',battery(),'pie central','Posición del objeto, valor +8; radio lógico .55.'),
 ('objeto_cartucho',cartridge(),'pie central','Posición del objeto, valor -2; radio lógico .9.'),
 ]
 for name,m,pivot,usage in specs:save(name,m,pivot,usage)
 (ROOT/'manifest.json').write_text(json.dumps({'minijuego':'Grúa de Chatarra','base':'claude/expansion-party ea5472c','coordenadas':'Y arriba, una unidad GLB = una unidad raylib','modelos':MANIFEST},ensure_ascii=False,indent=2))
 print(len(specs),'GLB;',sum(v['triangulos'] for v in MANIFEST),'triángulos')

if __name__=='__main__':main()
