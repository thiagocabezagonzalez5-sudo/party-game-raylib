"""Modelos originales GLB 2.0 para Bateo Meteórico."""
from geometria_glb import *
import geometria_glb as glb

glb.PALETTE.update({
 'roca':'#424b65','roca_clara':'#68708d','roca_oscura':'#262e47',
 'basalto':'#323a55','metal':'#a8b3c9','metal_claro':'#d6e1ef',
 'metal_oscuro':'#525c79','cobre':'#aa8064','oro':'#eac45c',
 'oro_claro':'#fff0a0','oro_oscuro':'#97744d',
 'azul':'#4381c0','violeta':'#9062c2','turquesa':'#6ae6d3',
 'meteor':'#a96d50','meteor_claro':'#d3976e','meteor_oscuro':'#744f49',
 'rojo':'#d84a4a','rojo_claro':'#ffa474','brasa':'#8f3b43',
 'planeta':'#dca978','planeta_claro':'#edc398','anillo':'#b9a69c',
 'negro':'#171d34','madera':'#986548','COLOR_DINAMICO':'#ffffff',
 'BOMBILLAS':'#fff1ba'
})

def hemisferio(m,center,r,mat,lat=9,lon=32,slot=False):
 x,y,z=center
 for j in range(lat):
  t0=(math.pi/2)*j/lat;t1=(math.pi/2)*(j+1)/lat
  for i in range(lon):
   a=TAU*i/lon;b=TAU*(i+1)/lon
   # Apertura del observatorio mirando hacia +Z.
   if slot and abs(math.sin((a+b)*.5))<.17 and math.cos((a+b)*.5)>.2:continue
   p=lambda t,u:(x+r*math.sin(t)*math.sin(u),y+r*math.cos(t),z+r*math.sin(t)*math.cos(u))
   quad(m,p(t0,a),p(t1,a),p(t1,b),p(t0,b),mat)

def cumbre():
 m=mesh()
 box(m,(0,-1.20,-25),(90,2,90),'roca_oscura')
 box(m,(0,-1,-70),(160,1,40),'basalto')
 for x in range(-42,43,6):
  for z in range(-63,16,9):
   if (x*13+z*7)%11<3:
    box(m,(x,.015,z),(1.5,.04,.8),'roca')
 for x in [-43,43]:
  for z in range(-60,12,8):
   ellipsoid(m,(x,.05,z),(.6,.15,.9),'roca_clara',8,4)
 return m

def cupula():
 m=mesh()
 cylinder(m,(0,-4,0),15,11,'metal_oscuro',32)
 for y in [-3.9,-1.3,2.0,5.1,6.8]:
  torus(m,(0,y,0),15.05,.13,'oro_oscuro',40,6)
 hemisferio(m,(0,7,0),15,'roca',10,40,True)
 for a in range(0,40,5):
  phi=TAU*a/40
  beam(m,(14.95*math.sin(phi),7,14.95*math.cos(phi)),
       (0,22,0),.10,'metal_claro',5)
 box(m,(4,15,9),(4,14,8),'negro')
 for x in [-5,5]:
  for z in [10,14]:
   box(m,(x,4,z),(.65,4.2,.65),'roca_clara')
   box(m,(x,6.15,z),(.95,.16,.95),'oro')
 return m

def telescopio():
 m=mesh()
 # Origen en centro de observatorio; tubo dirigido hacia el cielo.
 a=(4,12,9);b=(16,34,-6)
 beam(m,a,b,2.2,'metal',16,r_end=3.2)
 beam(m,(14.4,31,-3.6),(17,35.5,-7),3.43,'oro',16,r_end=3.57)
 beam(m,(17,35.5,-7),(17.25,35.8,-7.35),2.62,'negro',16,r_end=2.62)
 for i in range(4):
  k=(i+1)/5
  p=add(a,mul(sub(b,a),k))
  ellipsoid(m,p,(2.32+k*.65,.13,2.2+k*.64),'metal_oscuro',12,5)
 return m

def cupula_secundaria():
 m=mesh()
 cylinder(m,(0,-4,0),10,9,'metal_oscuro',24)
 hemisferio(m,(0,5,0),10,'roca',8,28)
 for y in [-3.9,4.9,8.1]:
  torus(m,(0,y,0),10.03,.11,'oro_oscuro',28,5)
 box(m,(0,8,9.5),(2.5,10,2),'negro')
 return m

def planeta():
 m=mesh()
 ellipsoid(m,(0,0,0),(13,13,13),'planeta',32,18)
 for y in [-5,-1,3,7]:
  rr=math.sqrt(max(1,13*13-y*y))
  torus(m,(0,y,0),rr+.05,.15,'planeta_claro',40,6)
 # Dos bandas inclinadas concéntricas para leer desde cámara baja.
 for radius,tube in [(18,.5),(20.2,.75),(22.6,.42),(24.5,.23)]:
  ringmesh=mesh();torus(ringmesh,(0,0,0),radius,tube,'anillo',56,7)
  for mat,ts in ringmesh.items():
   for tri0 in ts:
    m[mat].append(tuple((v[0],v[1]*.9165+v[2]*.40,-v[1]*.40+v[2]*.9165) for v in tri0))
 for i in range(20):
  a=TAU*i/20
  ellipsoid(m,(11.5*math.sin(a),2.5,11.5*math.cos(a)),(.22,.12,.24),'planeta_claro',8,4)
 return m

def farol():
 m=mesh()
 cylinder(m,(0,0,0),.15,1.76,'metal_oscuro',10,r_top=.085)
 cylinder(m,(0,1.76,0),.26,.08,'oro_oscuro',10)
 ellipsoid(m,(0,1.97,0),(.21,.21,.21),'BOMBILLAS',12,6)
 for x in [-.16,.16]:
  for z in [-.16,.16]:
   beam(m,(x,1.78,z),(x,2.15,z),.018,'metal',5)
 cylinder(m,(0,2.15,0),.27,.09,'metal_oscuro',10,r_top=.14)
 return m

def plataforma():
 m=mesh()
 # Origen sobre X del carril; piso Y=0; Z de -15 a +5.
 box(m,(0,-.2,-5),(5.4,.4,20),'roca')
 for i in range(10):
  z=-14+i*2
  box(m,(0,.01,z),(5.25,.02,.065),'roca_clara')
  for x in [-2.48,2.48]:
   box(m,(x,.02,z),(.10,.035,.25),'metal_oscuro')
 for x in [-2.6,2.6]:
  box(m,(x,.025,-5),(.16,.06,19.9),'COLOR_DINAMICO')
 for z in [-14.9,4.9]:
  beam(m,(-2.69,-.38,z),(2.69,-.38,z),.09,'basalto',6)
 return m

def campo():
 m=mesh()
 # Zona Z=-30..-14; 100/60/30 puntos centrada en -20.5.
 box(m,(0,-.25,-22),(5.4,.3,16),'basalto')
 for length,width,material,y in [(16,4.98,'azul',-.085),(8.4,4.8,'violeta',-.066),(3.6,4.6,'oro',-.046)]:
  box(m,(0,y,-20.5),(width,.018,length),material)
 for x in [-2.56,2.56]:
  box(m,(x,-.055,-22),(.12,.045,16),'COLOR_DINAMICO')
 for z in [-28.3,-24.7,-22.3,-18.7,-16.3,-12.6]:
  if z<-30 or z>-14:continue
  box(m,(0,-.031,z),(4.65,.018,.075),'metal_claro')
 for x in [-1.8,1.8]:
  torus(m,(x,-.020,-20.5),.23,.025,'oro_claro',18,5)
 return m

def canon_base():
 m=mesh()
 # Pivote al suelo, centro de pedestal en Z=0; canon en Z=-12.8 del mundo.
 box(m,(0,.20,0),(2.8,.4,2.8),'metal_oscuro')
 box(m,(0,2.0,0),(2,3.6,2),'roca')
 for y in [.30,2.4,3.8]:
  box(m,(0,y,.98),(2.2,.13,.16),'metal')
 ellipsoid(m,(0,4.70,.40),(.95,.95,.95),'metal',14,8)
 for x in [-1.01,1.01]:
  for y in [1.0,3.1]:
   box(m,(x,y,.52),(.15,.40,.16),'oro_oscuro')
 return m

def canon_tubo():
 m=mesh()
 # Pivote de boca detrás del proyectil; eje local +Y, longitud 2.2.
 cylinder(m,(0,0,0),.56,1.93,'metal',16,r_top=.68)
 cylinder(m,(0,1.84,0),.75,.36,'oro_oscuro',16,r_top=.74)
 cylinder(m,(0,2.19,0),.55,.03,'negro',16)
 torus(m,(0,2.18,0),.72,.055,'oro',20,6)
 for i in range(8):
  a=i*TAU/8
  beam(m,(.54*math.cos(a),.3,.54*math.sin(a)),
       (.65*math.cos(a),1.75,.65*math.sin(a)),.025,'metal_oscuro',5)
 return m

def meteorito(tipo):
 m=mesh()
 if tipo=='normal':
  ellipsoid(m,(0,0,0),(.45,.44,.45),'meteor',14,9)
  for x,y,z in [(-.24,.12,.31),(.27,-.08,.28),(-.08,-.29,-.24)]:
   ellipsoid(m,(x,y,z),(.10,.06,.06),'meteor_oscuro',8,4)
  ellipsoid(m,(.13,.18,.34),(.11,.10,.08),'meteor_claro',8,4)
 elif tipo=='dorado':
  ellipsoid(m,(0,0,0),(.5,.5,.5),'oro',16,9)
  for a in range(8):
   ang=TAU*a/8
   ellipsoid(m,(.44*math.cos(ang),.12,.44*math.sin(ang)),(.07,.06,.07),'oro_claro',8,4)
  torus(m,(0,0,0),.57,.028,'oro_claro',24,5,plane='xy')
 else:
  ellipsoid(m,(0,0,0),(.50,.50,.50),'brasa',16,9)
  for a in range(7):
   ang=TAU*a/7
   beam(m,(.22*math.cos(ang),-.28,.22*math.sin(ang)),
        (.37*math.cos(ang+.6),.32,.37*math.sin(ang+.6)),.025,'rojo_claro',6)
  for x,y,z in [(.25,.14,.34),(-.27,-.12,.28)]:
   ellipsoid(m,(x,y,z),(.10,.07,.06),'rojo',8,4)
 return m

def bate():
 m=mesh()
 # Pivote del swing; bate hacia Z negativo hasta -1.45.
 beam(m,(0,0,0),(0,0,-.51),.063,'madera',10,r_end=.074)
 beam(m,(0,0,-.50),(0,0,-1.38),.10,'COLOR_DINAMICO',12,r_end=.16)
 ellipsoid(m,(0,0,-1.39),(.16,.15,.10),'COLOR_DINAMICO',10,6)
 for z in [-.13,-.22,-.31,-.40]:
  torus(m,(0,0,z),.067,.008,'metal_oscuro',12,4,plane='xy')
 return m

def punto_dulce():
 m=mesh()
 # Aro visible de frente en plano XY, centro meteorito a Y=1.4.
 torus(m,(0,0,0),.55,.045,'turquesa',30,6,plane='xy')
 for a in [0,math.pi/2,math.pi,math.pi*1.5]:
  x,y=.55*math.cos(a),.55*math.sin(a)
  ellipsoid(m,(x,y,0),(.04,.04,.04),'BOMBILLAS',8,4)
 return m

def main():
 specs=[
 ('cumbre',cumbre(),'centro del mundo','Mundo (0,0,0); roca base detrás de carriles.'),
 ('observatorio_cupula',cupula(),'centro XZ de cúpula','Mundo (-36,0,-64); diámetro 30, altura máxima 22.'),
 ('telescopio',telescopio(),'centro de cúpula grande','Misma posición (-36,0,-64), tubo orientado hacia cielo.'),
 ('observatorio_secundario',cupula_secundaria(),'centro XZ de cúpula','Mundo (44,0,-76); diámetro 20.'),
 ('planeta_anillado',planeta(),'centro del planeta','Mundo (34,34,-112); radio de esfera 13, anillos hasta 25.'),
 ('farol',farol(),'suelo entre carriles','Mundo X=-15,-9,-3,3,9; Z=3.2.'),
 ('carril_plataforma',plataforma(),'X centro de carril, suelo Y=0','Mundo (carrilX,0,0); Z=-15..+5, ancho 5.4.'),
 ('campo_puntaje',campo(),'X centro de carril, suelo Y=0','Mundo (carrilX,0,0); zona Z=-30..-14.'),
 ('canon_base',canon_base(),'base pedestal en suelo','Mundo (carrilX,0,-12.8); hombro local (0,4.7,+.4).'),
 ('canon_tubo',canon_tubo(),'inicio eje vertical +Y','Orientar desde (carrilX,4.7,-12.4) a direccion del punto dulce; largo 2.2.'),
 ('meteorito_normal',meteorito('normal'),'centro','PosicionMeteoritoEntrante/Golpeado; radio .45.'),
 ('meteorito_dorado',meteorito('dorado'),'centro','Misma lógica, radio .50, puntaje doble.'),
 ('meteorito_rojo',meteorito('rojo'),'centro','Misma lógica, inestable, no golpear.'),
 ('bate',bate(),'inicio del swing','Mundo (carrilX+.4,1.05,.7), rotación Y ya calculada.'),
 ('aro_punto_dulce',punto_dulce(),'centro del aro','Mundo (carrilX,1.4,-.7), orientado perpendicular a Z.'),
 ]
 for name,model,pivot,usage in specs:save(name,model,pivot,usage)
 (glb.ROOT/'manifest.json').write_text(json.dumps({
  'version':1,'rama_referencia':'claude/expansion-party','commit_referencia':'ea5472c',
  'unidades':'1 unidad GLB = 1 unidad raylib','ejes':'Y arriba; jugador mira hacia Z negativo',
  'licencia':'Arte original procedural sin recursos externos',
  'nota':'La ventana de bateo, trayectorias, tipos y puntaje siguen en el código.',
  'modelos':glb.MANIFEST},ensure_ascii=False,indent=2),encoding='utf8')
 print(len(glb.MANIFEST),'GLB;',sum(x['triangulos'] for x in glb.MANIFEST),'triangulos')

if __name__=='__main__':main()
