"""Modelos originales GLB 2.0 para Racimo Tóxico."""
from geometria_glb import *
import geometria_glb as glb

glb.PALETTE.update({
 'agua':'#244c45','agua_clara':'#4b7970','espuma':'#82aaa0',
 'madera':'#806146','madera_clara':'#a27750','madera_oscura':'#463b36',
 'lodo':'#42493b','corteza':'#574637','corteza_clara':'#806448',
 'hoja':'#4c8152','hoja_clara':'#82aa67','hoja_oscura':'#315d42',
 'pasto':'#637e50','piedra':'#657269','piedra_clara':'#85968b',
 'fruto':'#b0ca58','fruto_claro':'#deea89','fruto_oscuro':'#758f40',
 'veneno':'#994ab2','veneno_claro':'#e974be','veneno_oscuro':'#522b75',
 'oro':'#f1cf60','oro_claro':'#fff3a4','oro_oscuro':'#b18a40',
 'luz':'#e4fa9c','marco':'#273c31','negro':'#162721',
 'COLOR_DINAMICO':'#ffffff','BOMBILLAS':'#fff7b9'
})

def doble(m,a,b,c,d,mat):
 quad(m,a,b,c,d,mat);quad(m,d,c,b,a,mat)

def agua():
 m=mesh()
 # Superficie y=-.10, ancho 90, fondo 70, centro Z=-4.
 box(m,(0,-.26,-4),(90,.32,70),'agua')
 for x in range(-40,41,8):
  for z in range(-36,29,8):
   if (x*3+z*5)%7<4:
    torus(m,(x+.5,-.092,z-.3),.85,.025,'agua_clara',18,5)
    torus(m,(x+.5,-.089,z-.3),1.17,.013,'espuma',18,4)
 for x,z in [(-14,-3),(-11,-6),(12,-4),(14,2),(-5,-10),(6,-11)]:
  ellipsoid(m,(x,-.075,z),(1.1,.07,.45),'lodo',10,4)
 return m

def arbol():
 m=mesh()
 # Pie del árbol en origen, mundo (-4.8,0,-1).
 cylinder(m,(0,-.15,0),.90,.38,'lodo',10)
 cylinder(m,(0,.03,0),.78,10.95,'corteza',9,r_top=.50)
 for y in [1.0,2.8,5.6,8.4]:
  beam(m,(-.54,y,-.02),(-.32,y+.62,.12),.055,'corteza_clara',6)
  beam(m,(.46,y+.25,.16),(.22,y+.82,.06),.048,'madera_oscura',6)
 # Rama que termina sobre el racimo en el origen mundial.
 beam(m,(.2,10.4,0),(4.8,11.3,1),.28,'corteza',9,r_end=.2)
 beam(m,(.1,8.5,0),(-2.7,10.5,-.5),.20,'corteza',7,r_end=.12)
 for a,b in [((1.0,10.6,.1),(1.1,9.3,.15)),
             ((2.9,11.0,.7),(2.85,9.9,.7)),
             ((-1.7,9.8,-.3),(-2.3,8.8,-.2))]:
  beam(m,a,b,.04,'hoja_oscura',6)
 for x,y,z in [(1.2,9.4,.16),(3.0,10.0,.7),(-2.4,8.9,-.2)]:
  ellipsoid(m,(x,y,z),(.17,.06,.10),'hoja',8,4)
 return m

def enredadera():
 m=mesh()
 # Base del tallo en Y=1.20; punta al final de la rama Y=11.3.
 beam(m,(0,1.2,0),(0,11.3,0),.095,'hoja_oscura',8,r_end=.075)
 for i in range(22):
  y=1.5+i*.42;lado=-1 if i%2==0 else 1
  beam(m,(0,y,0),(lado*.38,y,.01),.031,'hoja',6,r_end=.017)
  if i%3==0:
   ellipsoid(m,(lado*.20,y+.13,-.07),(.15,.10,.05),'hoja_clara',8,4)
 for y in [2.5,4.9,7.3,9.8]:
  torus(m,(0,y,0),.13,.022,'hoja_clara',12,4)
 return m

def fruto(tipo):
 m=mesh()
 rgb={'normal':('fruto','fruto_claro'),'toxico':('veneno','veneno_claro'),
      'dorado':('oro','oro_claro')}[tipo]
 ellipsoid(m,(0,0,0),(.20,.20,.20),rgb[0],16,8)
 beam(m,(0,.18,0),(0,.26,0),.024,'corteza',7)
 ellipsoid(m,(-.075,.22,0),(.095,.032,.055),'hoja',9,4)
 if tipo=='toxico':
  for x,y,z in [(-.13,.07,.14),(.11,.1,.14),(.04,-.11,.16)]:
   ellipsoid(m,(x,y,z),(.035,.041,.025),'veneno_claro',8,4)
  torus(m,(0,-.01,0),.205,.014,'veneno_claro',18,4,plane='xy')
 elif tipo=='dorado':
  for i in range(6):
   a=TAU*i/6
   ellipsoid(m,(.176*math.cos(a),.03,.176*math.sin(a)),(.035,.035,.035),'oro_claro',8,4)
  torus(m,(0,0,0),.226,.014,'oro_claro',18,4,plane='xy')
 else:
  for x,y,z in [(-.07,.06,.175),(.08,-.08,.17)]:
   ellipsoid(m,(x,y,z),(.04,.04,.02),'fruto_claro',8,4)
 return m

def balsa():
 m=mesh()
 # Pivote en centro XZ, tabla centrada en Y=0; cuatro listones 3.2x.5.
 for i in range(4):
  z=-.8+i*.55
  box(m,(0,0,z),(3.20,.24,.50),'madera' if i%2==0 else 'madera_oscura')
  box(m,(0,.125,z),(3.14,.012,.44),'madera_clara' if i%2==0 else 'madera')
  for x in [-1.46,1.46]:
   for zz in [-.14,.14]:
    ellipsoid(m,(x,.135,z+zz),(.025,.016,.025),'oro_oscuro',6,4)
 for x in [-1.18,1.18]:
  box(m,(x,-.18,.03),(.19,.14,2.12),'madera_oscura')
 for z in [-.80,.85]:
  box(m,(0,.17,z),(3.25,.08,.06),'hoja_oscura')
 for x in [-1.48,1.48]:
  torus(m,(x,.02,.03),.17,.022,'oro_oscuro',14,5,plane='yz')
 return m

def tronco():
 m=mesh()
 # Eje local +Z, longitud unitaria para escalar a distancia de extremos.
 beam(m,(0,0,0),(0,0,1),.32,'corteza',9,r_end=.30)
 for z in [.08,.46,.88]:
  torus(m,(0,0,z),.31,.029,'madera_oscura',16,5,plane='xy')
 for z in [.23,.57,.76]:
  ellipsoid(m,(0,.30,z),(.11,.055,.18),'hoja',9,4)
 for x in [-.12,.12]:
  beam(m,(x,.27,.20),(x*.72,.26,.82),.036,'hoja_oscura',6)
 return m

def cabana():
 m=mesh()
 # Pie al nivel del agua, cabaña y entrepiso desde Y=1.9.
 for x in [-1.2,1.2]:
  for z in [-.9,.9]:
   cylinder(m,(x,.60,z),.14,2.30,'madera_oscura',7,r_top=.12)
   ellipsoid(m,(x,.55,z),(.21,.11,.21),'piedra',8,4)
 box(m,(0,1.9,0),(3.2,.18,2.4),'madera')
 for x in [-1.2,-.6,0,.6,1.2]:
  box(m,(x,2.00,0),(.05,.02,2.32),'madera_clara')
 box(m,(0,2.8,0),(2.8,1.6,2.0),'madera')
 for x in [-1.22,1.22]:
  box(m,(x,2.79,1.017),(.13,1.55,.08),'madera_oscura')
 box(m,(0,2.82,1.035),(.70,.70,.05),'oro_claro')
 box(m,(0,2.82,1.067),(.48,.51,.011),'BOMBILLAS')
 # Tejado a dos aguas, alero sobre z y x.
 a=(-1.62,3.56,-1.3);b=(1.62,3.56,-1.3)
 c=(1.62,3.56,1.3);d=(-1.62,3.56,1.3)
 r0=(0,4.75,-1.3);r1=(0,4.75,1.3)
 doble(m,b,c,r1,r0,'hoja_oscura')
 doble(m,d,a,r0,r1,'hoja')
 tri(m,a,b,r0,'hoja_oscura');tri(m,r0,b,a,'hoja_oscura')
 tri(m,c,d,r1,'hoja_oscura');tri(m,r1,d,c,'hoja_oscura')
 beam(m,r0,r1,.09,'madera_oscura',7)
 for z in [-1.2,1.2]:
  beam(m,(-1.62,3.56,z),(0,4.75,z),.045,'madera_clara',5)
  beam(m,(0,4.75,z),(1.62,3.56,z),.045,'madera_clara',5)
 return m

def arbol_fondo():
 m=mesh()
 cylinder(m,(0,0,0),.72,8,'corteza',9,r_top=.48)
 ellipsoid(m,(0,8.6,0),(2.0,2.0,1.8),'hoja_oscura',11,6)
 for x,z in [(-1,.1),(.8,.2),(0,-1)]:
  ellipsoid(m,(x,8.3,z),(1.2,1.2,1.0),'hoja',10,5)
 for i in range(5):
  a=TAU*i/5
  beam(m,(.4*math.cos(a),7.7,.4*math.sin(a)),
       (1.25*math.cos(a),6.0,1.25*math.sin(a)),.025,'hoja_oscura',5)
 return m

def luciernaga():
 m=mesh()
 ellipsoid(m,(0,0,0),(.075,.075,.075),'luz',10,5)
 torus(m,(0,0,0),.145,.011,'BOMBILLAS',14,4,plane='xy')
 ellipsoid(m,(0,-.08,0),(.05,.07,.05),'madera_oscura',8,4)
 return m

def nenufar():
 m=mesh()
 cylinder(m,(0,0,0),.48,.04,'hoja',18)
 for i in range(5):
  a=TAU*i/5
  ellipsoid(m,(.24*math.cos(a),.05,.24*math.sin(a)),(.19,.025,.12),'hoja_clara',8,4)
 ellipsoid(m,(0,.13,0),(.15,.10,.15),'oro',10,5)
 return m

def roca():
 m=mesh();ellipsoid(m,(0,.09,0),(.72,.32,.55),'piedra',10,5)
 ellipsoid(m,(.18,.28,-.11),(.32,.23,.30),'piedra_clara',9,5)
 for x,z in [(-.3,.19),(.24,.24)]:
  ellipsoid(m,(x,.32,z),(.13,.035,.10),'hoja_oscura',8,4)
 return m

def aro_turno():
 m=mesh();torus(m,(0,0,0),2.3,.045,'oro',32,5)
 for i in range(8):
  a=TAU*i/8
  ellipsoid(m,(2.3*math.cos(a),0,2.3*math.sin(a)),(.07,.05,.07),'BOMBILLAS',8,4)
 return m

def main():
 specs=[
 ('agua_pantano',agua(),'centro de pantano','Mundo (0,0,0); superficie Y=-.1 y centro Z=-4.'),
 ('arbol_podrido',arbol(),'pie del arbol','Mundo (-4.8,0,-1); rama termina sobre enredadera central.'),
 ('enredadera_central',enredadera(),'centro de racimo en suelo','Mundo (0,0,0); ramas laterales coinciden con frutos y=1.5+.42*i.'),
 ('fruto_normal',fruto('normal'),'centro de baya','PosicionFrutaRacimo(i,t); radio .2.'),
 ('fruto_toxico',fruto('toxico'),'centro de baya','Indices 5,11,17; centro de fruto segun estado.'),
 ('fruto_dorado',fruto('dorado'),'centro de baya','Indice de premio aleatorio, centro segun estado.'),
 ('balsa',balsa(),'centro del listonado Y=0','Mundo (posicionBalsaX,0.08+hundimiento+bamboleo,3.5); ancho 3.2.'),
 ('tronco_flotante',tronco(),'inicio de tronco sobre +Z','Repetir entre extremos de DibujarTroncoFlotanteRacimo orientando +Z y escalando largo.'),
 ('cabana_pilotes',cabana(),'base al nivel del agua','Mundo (-10.5,0,-8) y (9.5,0,-9).'),
 ('arbol_fondo',arbol_fondo(),'base al suelo','Mundo X=-16+5.5*i, Z=-15 para i=0..6.'),
 ('luciernaga',luciernaga(),'centro de insecto','Instanciar 16 según posiciones animadas; brillo por tamaño o material.'),
 ('nenufar',nenufar(),'base al agua','Decoración libre en Y=-.08, fuera de las balsas.'),
 ('roca_pantano',roca(),'base al agua','Decoración libre, fuera del espacio jugable.'),
 ('aro_turno',aro_turno(),'centro en plano agua','Mundo (posicionBalsaX,.02,3.5) cuando activa.'),
 ]
 for name,model,pivot,usage in specs:save(name,model,pivot,usage)
 (glb.ROOT/'manifest.json').write_text(json.dumps({
  'version':1,'rama_referencia':'claude/expansion-party','commit_referencia':'ea5472c',
  'unidades':'1 unidad GLB = 1 unidad raylib','ejes':'Y arriba, cámara hacia Z negativo',
  'licencia':'Arte original procedural sin recursos externos',
  'nota':'La selección, veneno, turno, vuelos y hundimiento siguen en C++.',
  'modelos':glb.MANIFEST},ensure_ascii=False,indent=2),encoding='utf8')
 print(len(glb.MANIFEST),'GLB;',sum(x['triangulos'] for x in glb.MANIFEST),'triangulos')

if __name__=='__main__':main()
