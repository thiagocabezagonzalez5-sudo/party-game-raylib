"""Modelos originales GLB 2.0 para Cápsulas Barajadas."""
from geometria_glb import *
import geometria_glb as glb

glb.PALETTE.update({
 'suelo':'#aebbc4','suelo_claro':'#ced5d9','junta':'#8798a5',
 'pared':'#426174','pared_oscura':'#263e50','acero':'#b8c8d4',
 'acero_claro':'#e2e9e9','acero_oscuro':'#566776',
 'carbon':'#242d37','negro':'#111d2b','cristal':'#a9e1e4',
 'pantalla':'#0c4141','menta':'#6af3b8','menta_clara':'#c4ffe4',
 'naranja':'#ed913d','naranja_oscuro':'#a85d31',
 'oro':'#f3cb62','amarillo':'#f5d549','rojo':'#df6a5b',
 'azul':'#68b8ef','violeta':'#b28be1','rosa':'#ec98c5',
 'COLOR_DINAMICO':'#ffffff','BOMBILLAS':'#ffffff'
})

def tubo_hueco(m,p,r,ri,h,mat,inner='carbon',n=24):
 x,y,z=p
 for i in range(n):
  a=i*TAU/n;b=(i+1)*TAU/n
  q=lambda radius,ang,height:(x+radius*math.cos(ang),y+height,z+radius*math.sin(ang))
  quad(m,q(r,a,0),q(r,b,0),q(r,b,h),q(r,a,h),mat)
  quad(m,q(ri,b,0),q(ri,a,0),q(ri,a,h),q(ri,b,h),inner)
  quad(m,q(r,a,h),q(r,b,h),q(ri,b,h),q(ri,a,h),mat)

def sala():
 m=mesh();box(m,(0,-.20,0),(40,.40,40),'suelo')
 for k in range(-9,10):
  box(m,(k*2,.012,0),(.035,.015,30),'junta')
  box(m,(0,.012,k*2),(26,.015,.035),'junta')
 for x in range(-12,13,2):
  for z in range(-8,11,2):
   if (x+z*3)%4==0:
    box(m,(x+.71,.027,z+.72),(.19,.012,.19),'suelo_claro')
 box(m,(0,4,-8.4),(26,8,.4),'pared')
 for y in [1.2,2.8,7.3]:
  box(m,(0,y,-8.17),(25.7,.10,.08),'acero_oscuro')
 for x in range(-12,13,2):
  box(m,(x,4,-8.16),(.06,7.8,.08),'pared_oscura')
 for x in [-11,11]:
  box(m,(x,6.7,-8.13),(.36,.65,.035),'menta')
 return m

def monitor():
 m=mesh()
 # Pivote centro de pantalla, frente +Z; instalar en Z=-8.1.
 box(m,(0,0,0),(3.5,2.12,.18),'carbon')
 box(m,(0,0,.103),(3.14,1.76,.032),'pantalla')
 for b in range(6):
  h=.27+((b*5+3)%7)*.09
  x=-1.23+b*.48
  box(m,(x,-.7+h/2,.125),(.29,h,.017),'menta')
 box(m,(0,.7,.126),(2.8,.035,.017),'azul')
 for x in [-1.6,1.6]:
  for y in [-.90,.90]:ellipsoid(m,(x,y,.11),(.035,.035,.017),'oro',8,4)
 return m

def mostrador():
 m=mesh()
 box(m,(0,.52,0),(22,1.04,1.4),'acero_oscuro')
 box(m,(0,1.08,0),(22.25,.13,1.6),'acero')
 box(m,(0,.68,.711),(21.4,.46,.025),'pared')
 for x in range(-10,11,2):
  box(m,(x,.68,.731),(.42,.08,.02),'menta')
 for x in [-10.9,10.9]:
  box(m,(x,1.14,0),(.16,.05,1.62),'carbon')
 return m

def ensayo():
 m=mesh()
 # Recipiente abierto con líquido hasta y=.9 desde el mostrador.
 cylinder(m,(0,0,0),.36,.07,'acero_oscuro',14)
 cylinder(m,(0,.06,0),.25,.74,'COLOR_DINAMICO',16)
 tubo_hueco(m,(0,.05,0),.30,.272,1.46,'cristal','cristal',20)
 torus(m,(0,1.51,0),.29,.038,'acero_claro',24,6)
 for y in [.33,.64,.98]:
  ellipsoid(m,(.10,y,.06),(.045,.056,.043),'BOMBILLAS',8,5)
 return m

def baliza():
 m=mesh()
 cylinder(m,(0,0,0),.30,.48,'carbon',12)
 torus(m,(0,.49,0),.33,.05,'acero',20,6)
 ellipsoid(m,(0,.82,0),(.34,.34,.34),'COLOR_DINAMICO',16,8)
 for i in range(6):
  a=i*TAU/6
  beam(m,(.30*math.cos(a),.55,.30*math.sin(a)),(.30*math.cos(a),1.05,.30*math.sin(a)),.025,'acero',5)
 cylinder(m,(0,1.13,0),.22,.11,'carbon',12,r_top=.15)
 return m

def mesa():
 m=mesh()
 # Encima Y=1.01; ancho 11.4, profundo 3.6.
 box(m,(0,.95,0),(11.4,.12,3.6),'acero')
 box(m,(0,.89,0),(11.2,.055,3.4),'carbon')
 box(m,(0,1.018,0),(11.0,.018,3.14),'acero_claro')
 for x in [-5.3,5.3]:
  for z in [-1.5,1.5]:
   cylinder(m,(x,0,z),.17,.88,'acero_oscuro',10,r_top=.14)
   cylinder(m,(x,.07,z),.22,.11,'carbon',10)
 for z in [-1.68,1.68]:
  box(m,(0,1.024,z),(11.12,.025,.085),'menta')
 for x in [-5.4,5.4]:
  box(m,(x,1.022,0),(.08,.025,3.3),'menta')
 for x in [-4.25,-2.1,0,2.1,4.25]:
  torus(m,(x,1.031,0),.65,.02,'acero_oscuro',24,5)
 return m

def pasarela():
 m=mesh()
 box(m,(0,.15,0),(11,.30,2.6),'acero_oscuro')
 box(m,(0,.304,0),(10.85,.011,2.46),'acero')
 for i in range(22):
  box(m,(-5.25+i*.5,.318,-1.20),(.5,.013,.20),'amarillo' if i%2==0 else 'carbon')
 for x in [-5.3,5.3]:
  for z in [-1.20,1.20]:
   box(m,(x,.34,z),(.12,.04,.11),'carbon')
 return m

def brazo_base():
 m=mesh()
 cylinder(m,(0,0,0),.83,.13,'carbon',16)
 cylinder(m,(0,.13,0),.68,3.45,'acero_oscuro',16,r_top=.51)
 for y in [.40,1.7,3.2]:
  torus(m,(0,y,0),.60-(y*.02),.065,'naranja',24,6)
 cylinder(m,(0,3.58,0),.50,.25,'carbon',12)
 ellipsoid(m,(0,4.2,0),(.44,.44,.44),'naranja',16,8)
 return m

def segmento():
 m=mesh()
 # Origen en unión inicial, extremo Y=1; escalar únicamente eje Y.
 cylinder(m,(0,.05,0),.22,.88,'naranja',12,r_top=.18)
 for x in [-.19,.19]:
  beam(m,(x,.12,0),(x,.89,0),.025,'acero_oscuro',6)
 cylinder(m,(0,0,0),.25,.10,'carbon',12)
 cylinder(m,(0,.90,0),.21,.11,'carbon',12)
 return m

def articulacion():
 m=mesh();ellipsoid(m,(0,0,0),(.30,.30,.30),'carbon',12,7)
 torus(m,(0,0,0),.29,.045,'naranja',20,6,plane='xy')
 for x in [-.27,.27]:
  ellipsoid(m,(x,0,0),(.065,.09,.09),'acero',10,6)
 return m

def pinza():
 m=mesh()
 box(m,(0,-.03,0),(.54,.20,.33),'naranja')
 for x in [-.21,.21]:
  box(m,(x,-.21,0),(.08,.39,.22),'carbon')
  box(m,(x*.83,-.42,0),(.11,.11,.21),'acero')
 cylinder(m,(0,.09,0),.16,.14,'acero_oscuro',10)
 return m

def capsula():
 m=mesh()
 # Cuerpo hueco de Y=0 a Y=1.5, exterior radio .8.
 cylinder(m,(0,0,0),.8,.10,'carbon',24)
 tubo_hueco(m,(0,.10,0),.8,.69,1.40,'acero','acero_oscuro',24)
 torus(m,(0,1.51,0),.75,.055,'carbon',24,6)
 for i in range(8):
  a=i*TAU/8
  x,z=.805*math.cos(a),.805*math.sin(a)
  beam(m,(x,.18,z),(x,1.41,z),.018,'acero_oscuro',5)
 for y in [.22,1.28]:
  torus(m,(0,y,0),.795,.028,'acero_claro',24,6)
 return m

def banda():
 m=mesh()
 # Banda separada, material dinámico sin teñir el cuerpo.
 tubo_hueco(m,(0,.45,0),.838,.829,.28,'COLOR_DINAMICO','COLOR_DINAMICO',24)
 for i in range(12):
  a=i*TAU/12
  ellipsoid(m,(.84*math.cos(a),.59,.84*math.sin(a)),(.024,.03,.024),'BOMBILLAS',7,4)
 return m

def tapa():
 m=mesh()
 # Pivote en borde inferior; base en Y=0; posición a y0+1.5+apertura*1.3.
 cylinder(m,(0,0,0),.82,.09,'carbon',24)
 cylinder(m,(0,.09,0),.79,.19,'acero',24,r_top=.59)
 cylinder(m,(0,.28,0),.52,.075,'acero_claro',18)
 ellipsoid(m,(0,.45,0),(.14,.14,.14),'COLOR_DINAMICO',12,6)
 for a in [0,1.57,3.14,4.71]:
  x,z=.68*math.cos(a),.68*math.sin(a)
  box(m,(x,.27,z),(.09,.06,.10),'carbon')
 return m

def nucleo():
 m=mesh()
 ellipsoid(m,(0,0,0),(.32,.32,.32),'menta',16,10)
 ellipsoid(m,(-.09,.11,-.12),(.075,.045,.07),'BOMBILLAS',10,5)
 for plane in ['xy','yz','xz']:
  torus(m,(0,0,0),.41,.018,'menta_clara',28,5,plane=plane)
 return m

def marcador():
 m=mesh()
 # Pivote suelo de pasarela. Cono Y=0..55.
 cylinder(m,(0,0,0),.24,.55,'COLOR_DINAMICO',12,r_top=0)
 torus(m,(0,.045,0),.25,.026,'carbon',18,5)
 ellipsoid(m,(0,.72,0),(.20,.20,.20),'COLOR_DINAMICO',12,7)
 return m

def main():
 specs=[
 ('sala_laboratorio',sala(),'centro del suelo','Mundo (0,0,0), pared posterior en Z=-8.4.'),
 ('monitor',monitor(),'centro de pantalla','X=-7.5,-2.5,2.5,7.5; Y=4.6; Z=-8.1, frente +Z.'),
 ('mostrador',mostrador(),'base del mostrador','Mundo (0,0,-6.4); largo 22.'),
 ('tubo_ensayo',ensayo(),'base del tubo','Mundo X=-8.4+2.4*k, Y=1, Z=-6.4 para k=0..7.'),
 ('baliza',baliza(),'base de baliza','Mundo (X+-10.5,5.6,-7.8); tintar segun subfase.'),
 ('mesa_acero',mesa(),'centro de mesa sobre suelo','Mundo (0,0,0); encimera Y=1.01.'),
 ('pasarela',pasarela(),'centro de base a suelo','Mundo (0,0,4.8); altura superior .3.'),
 ('brazo_base',brazo_base(),'base del robot','Mundo (0,0,-3.4); hombro Y=4.2.'),
 ('brazo_segmento',segmento(),'union inicial del brazo, apunta hacia +Y','Dibujar entre hombro-codo y codo-mano, orientar +Y y escalar longitud en Y.'),
 ('brazo_articulacion',articulacion(),'centro de articulacion','Colocar en codo calculado por PosicionManoCapsulas.'),
 ('brazo_pinza',pinza(),'centro de mano','Colocar en PosicionManoCapsulas.'),
 ('capsula_cuerpo',capsula(),'base de capsula','PosicionCapsulaCapsulas; altura 1.5, radio .8.'),
 ('capsula_banda',banda(),'base de capsula','Misma posicion, tinte individual solo en COLOR_DINAMICO.'),
 ('capsula_tapa',tapa(),'base de tapa','(p.x,p.y+1.5+tapaAbierta*1.3,p.z), misma elevacion revelada.'),
 ('nucleo',nucleo(),'centro del nucleo','Posicion del nucleo del minijuego; escala pulsante.'),
 ('marcador',marcador(),'suelo del marcador','En Z=1.5 sobre mesa; tinte por jugador.'),
 ]
 for name,model,pivot,usage in specs:save(name,model,pivot,usage)
 (glb.ROOT/'manifest.json').write_text(json.dumps({
  'version':1,'rama_referencia':'claude/expansion-party','commit_referencia':'ea5472c',
  'unidades':'1 unidad GLB = 1 unidad raylib','ejes':'Y arriba, frente sala hacia +Z',
  'licencia':'Arte original procedural sin recursos externos',
  'nota':'La elección, el barajado, las subfases y el puntaje siguen en C++.',
  'modelos':glb.MANIFEST},ensure_ascii=False,indent=2),encoding='utf8')
 print(len(glb.MANIFEST),'GLB;',sum(x['triangulos'] for x in glb.MANIFEST),'triangulos')

if __name__=='__main__':main()
