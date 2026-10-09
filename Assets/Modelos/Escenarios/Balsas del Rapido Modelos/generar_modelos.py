"""GLB procedurales originales para Balsas del Rápido. Python 3 estándar."""
from geometria_glb import *
import geometria_glb as glb

glb.PALETTE.update({
 'agua':'#28b8bb','agua_luz':'#50d3cf','agua_sombra':'#227e98','espuma':'#d8f9f4',
 'selva':'#245a38','selva_oscura':'#1e4934','musgo':'#4c9551','musgo_luz':'#70b761',
 'madera':'#ac7545','madera_luz':'#d19a56','madera_sombra':'#68482f','soga':'#e6c582',
 'piedra':'#858e91','piedra_luz':'#b3bdba','piedra_sombra':'#57636a',
 'hoja':'#2c994d','hoja_luz':'#4cb966','liana':'#288348',
 'banana':'#ffe05b','banana_sombra':'#d99a3b',
 'naranja':'#ff9e44','rosa':'#ee61ab','rojo':'#e85055','azul':'#5c90e3',
 'amarillo':'#f1cc59','blanco':'#fffdfa','negro':'#273141',
 'COLOR_DINAMICO':'#ffffff','BOMBILLAS':'#ffffff'})

def water():
 m=mesh();box(m,(0,-.15,0),(11.2,.3,20),'agua')
 box(m,(0,.005,0),(10.8,.012,19.96),'agua_luz')
 for i in range(22):
  x=-4.8+(i*17%95)/10;z=-9.7+(i*29%192)/10
  box(m,(x,.023,z),(.5+.2*(i%3),.015,.075),'espuma' if i%4==0 else 'agua')
 return m

def island():
 m=mesh();box(m,(0,.05,0),(4.8,.5,20),'musgo')
 box(m,(0,.32,0),(4.0,.06,20),'musgo_luz')
 for i in range(10):
  x=-1.55+(i*7%30)/10;z=-9.2+(i*19%185)/10
  ellipsoid(m,(x,.37,z),(.18,.08,.14),'selva',8,4)
 return m

def bank():
 m=mesh();box(m,(0,.6,0),(8,1.6,20),'selva')
 box(m,(7,3,0),(10,6,20),'selva_oscura')
 box(m,(-4.05,.02,0),(.3,.85,20),'piedra_sombra')
 for i in range(9):
  z=-9+i*2.25
  ellipsoid(m,(-3.83,.42,z),(.18,.12,.33),'piedra',8,4)
 return m

def tree():
 m=mesh();cylinder(m,(0,0,0),1.0,9.8,'madera',10,r_top=.69)
 for i in range(7):
  y=1.3+i*1.1;torus(m,(0,y,0),.87-.03*i,.035,'madera_sombra',12,4)
 ellipsoid(m,(0,10.9,0),(3.45,2.7,3.2),'hoja',13,7)
 ellipsoid(m,(2,9.3,1.4),(2.2,2.0,2.1),'hoja_luz',12,6)
 ellipsoid(m,(-1.8,11.4,-1.2),(1.8,1.4,1.5),'selva',11,5)
 for i in range(4):
  a=TAU*i/4
  beam(m,(2.6*math.cos(a),10.0,2.4*math.sin(a)),(2.5*math.cos(a),5.0+(i%3),2.4*math.sin(a)),.055,'liana',6)
 return m

def vine():
 m=mesh();beam(m,(0,5.3,0),(.2,.2,0),.06,'liana',7)
 for i in range(4):
  y=.65+i*1.2
  ellipsoid(m,(.2,y,.24),(.14,.26,.05),'hoja_luz',8,4)
 return m

def palm():
 m=mesh();cylinder(m,(0,0,0),.22,2.4,'madera',7,r_top=.13)
 for i in range(7):
  a=TAU*i/7;beam(m,(0,2.5,0),(1.05*math.cos(a),2.3,1.05*math.sin(a)),.11,'hoja',6,r_end=.015)
 ellipsoid(m,(0,2.6,0),(.4,.35,.4),'hoja_luz',10,5)
 return m

def ruin():
 m=mesh();box(m,(-1.2,1.8,0),(1,2.5,1),'piedra')
 box(m,(-1.2,3.1,0),(1.12,.16,1.12),'musgo')
 box(m,(1.1,1.4,.8),(1,1.6,1),'piedra_luz')
 box(m,(1.1,2.25,.8),(1.12,.15,1.12),'musgo')
 box(m,(0,.5,-1.2),(2.4,.4,.9),'piedra_sombra')
 for x in (-1.35,1.15):
  beam(m,(x,2.9,0),(x+.2,1.5,.35),.045,'liana',6)
 return m

def waterfall():
 m=mesh();box(m,(0,3.1,0),(.4,5.6,1.8),'agua_luz')
 for i in range(6):box(m,(.225,3.0,-.75+i*.3),(.022,5.2,.1),'espuma')
 ellipsoid(m,(-.5,.25,0),(.75,.27,1),'espuma',12,5)
 return m

def rapid():
 m=mesh();box(m,(0,.035,0),(2.6,.05,.52),'espuma')
 for i in range(8):
  x=-1.1+i*.3;ellipsoid(m,(x,.07,.21),(.16,.04,.12),'blanco',7,4)
 return m

def rock():
 m=mesh();ellipsoid(m,(0,.3,0),(.9,.68,.8),'piedra',11,6)
 ellipsoid(m,(.22,.68,-.12),(.42,.32,.43),'piedra_luz',9,5)
 box(m,(-.08,.81,0),(.93,.10,.7),'musgo')
 return m

def log():
 m=mesh();beam(m,(-1.7,.15,0),(1.7,.15,0),.42,'madera',10)
 for x in (-1.35,.25,1.35):torus(m,(x,.15,0),.42,.035,'madera_sombra',12,4,plane='yz')
 for x in (-1.69,1.69):ellipsoid(m,(x,.15,0),(.02,.31,.31),'madera_luz',10,5)
 return m

def divider():
 m=mesh();box(m,(0,.32,0),(1.4,1,30),'piedra_sombra')
 box(m,(0,.85,0),(1.5,.14,30),'musgo')
 for i in range(9):
  z=-13+i*3.4
  ellipsoid(m,((i%2-.5)*.22,.9,z),(.52,.33,.76),'piedra',9,5)
 return m

def whirlpool():
 m=mesh()
 for r in range(4):
  rad=2*(1-.2*r)
  torus(m,(0,.05+.01*r,0),rad,.044,'agua_sombra' if r%2 else 'espuma',28,5)
 for i in range(7):
  a=TAU*i/7
  ellipsoid(m,(1.65*math.cos(a),.08,1.65*math.sin(a)),(.09,.035,.12),'espuma',7,4)
 return m

def banana():
 m=mesh();beam(m,(-.28,.31,0),(0,.39,0),.12,'banana',8)
 beam(m,(0,.39,0),(.30,.67,0),.11,'banana',8,r_end=.05)
 ellipsoid(m,(-.28,.32,0),(.045,.045,.05),'banana_sombra',7,4)
 return m

def parrot(color):
 m=mesh();ellipsoid(m,(0,0,0),(.28,.32,.27),color,11,6)
 ellipsoid(m,(0,.28,-.18),(.17,.19,.17),color,10,5)
 box(m,(0,.21,-.4),(.14,.08,.18),'naranja')
 ellipsoid(m,(.10,.32,-.27),(.035,.04,.027),'negro',7,4)
 beam(m,(0,-.10,.2),(0,-.14,.75),.055,'azul',6)
 return m

def parrot_wing(color):
 m=mesh();ellipsoid(m,(0,0,0),(.50,.055,.24),color,9,5)
 for i in range(3):
  x=-.25+i*.22;ellipsoid(m,(x,-.015,.17),(.08,.035,.15),'azul',7,4)
 return m

def raft():
 m=mesh()
 for k in range(5):
  x=-.68+k*.34;box(m,(x,0,0),(.32,.22,2.6),'madera_luz' if k%2 else 'madera')
  for z in (-1.3,1.3):ellipsoid(m,(x,0,z),(.155,.11,.035),'madera_sombra',8,4)
 for z in (-.8,.8):box(m,(0,.14,z),(1.8,.08,.14),'madera_sombra')
 for z in (-.75,.75):
  for x in (-.68,.68):torus(m,(x,.185,z),.065,.022,'soga',10,4)
 beam(m,(0,.1,-1.15),(0,1.5,-1.15),.035,'madera_sombra',6)
 box(m,(.25,1.25,-1.15),(.5,.3,.05),'COLOR_DINAMICO')
 return m

def paddle():
 m=mesh();beam(m,(0,0,0),(.86,-.25,.30),.055,'madera',7)
 box(m,(.9,-.25,.30),(.14,.06,.55),'soga')
 ellipsoid(m,(0,0,0),(.075,.07,.07),'madera_sombra',8,4)
 return m

def finish():
 m=mesh()
 for x,w in [(-13.6,1),(0,1.2),(13.6,1)]:
  box(m,(x,2.6,0),(w,5.2,1),'piedra')
  box(m,(x,5.0,.53),(w+.1,.13,.08),'musgo')
 box(m,(0,5.4,0),(28.4,.7,1),'piedra_sombra')
 for k in range(20):
  x=-13.+k*26/19
  box(m,(x,5.08,.56),(1,.35,.07),'blanco' if k%2 else 'negro')
 return m

def finish_tile():
 m=mesh()
 for i in range(12):
  for j in range(2):box(m,(-5.05+i*.92,.04,-.45+j*.9),(.92,.03,.9),'blanco' if (i+j)%2 else 'negro')
 return m

def main():
 specs=[
 ('rio_tramo',water(),'centro de tramo','Dos carriles X=±8; repetir cada 20 en Z=−10,−30,...,−130.'),
 ('isla_tramo',island(),'centro de tramo','Isla central X=0; repetir cada 20.'),
 ('orilla_tramo',bank(),'centro de tramo','Copias en X=±17.6, Z=−10−20*i; reflejar con rotación Y=180° para orilla derecha.'),
 ('arbol_selva',tree(),'pie central','Árboles exteriores X≈±18..22 cada 7 unidades de avance.'),
 ('liana',vine(),'origen superior aproximado','Decoración animable de los árboles.'),
 ('palmera_isla',palm(),'pie central','Isla central, alrededor de X=±0.9 cada 11 de avance.'),
 ('ruina_musgosa',ruin(),'pie central','Copias en p=18,41,63,88,106 sobre isla.'),
 ('cascada_orilla',waterfall(),'pie central','Copias en X≈±13.8, p=30,95,113.'),
 ('espuma_rapido',rapid(),'centro','Instanciar en cada carril: X=centroCarril−3.2; p≈56..80.'),
 ('roca_obstaculo',rock(),'pie central','Centro (c+desvio,0,−p), escala según o.a; radio lógico independiente.'),
 ('tronco_flotante',log(),'centro','Centro (c+desvio,0,−p); longitud 2*o.a≈3.4.'),
 ('divisor_rapido',divider(),'centro','En cada carril: centro X=±8, p=67; ancho 2*o.a=1.4, largo 30.'),
 ('remolino',whirlpool(),'centro','Centro (c+desvio,0,−p), radio lógico 2; girar Y para animar.'),
 ('banana_boost',banana(),'pie central','Centro (c+desvio,0,−p); bob Y≈0.3 y ocultar al recoger.'),
 ('loro_rojo',parrot('rojo'),'centro de cuerpo','Pájaros junto a orillas, vuelo Y≈5.'),
 ('loro_azul',parrot('azul'),'centro de cuerpo','Pájaros junto a orillas, vuelo Y≈5.'),
 ('loro_amarillo',parrot('amarillo'),'centro de cuerpo','Pájaros junto a orillas, vuelo Y≈5.'),
 ('loro_ala_roja',parrot_wing('rojo'),'unión lateral','Dos alas animadas ±X para loro rojo.'),
 ('loro_ala_azul',parrot_wing('azul'),'unión lateral','Dos alas animadas ±X para loro azul.'),
 ('loro_ala_amarilla',parrot_wing('amarillo'),'unión lateral','Dos alas animadas ±X para loro amarillo.'),
 ('balsa',raft(),'centro','Instanciar en (b.x,0.12+oscilación,−b.p), girar −b.rumbo; teñir bandera.'),
 ('remo',paddle(),'eje de agarre','Dos por balsa; animar palada en cada lado, independiente del casco.'),
 ('arco_meta',finish(),'base en centro','Situar en (0,0,−120), cruza los dos carriles.'),
 ('linea_meta',finish_tile(),'centro','Dos copias en X=±8, Z=−120.'),
 ]
 for name,m,pivot,usage in specs:save(name,m,pivot,usage)
 (ROOT/'manifest.json').write_text(json.dumps({'minijuego':'Balsas del Rápido','base':'claude/expansion-party ea5472c','coordenadas':'Y arriba; Z=−avance; unidades raylib','modelos':MANIFEST},ensure_ascii=False,indent=2))
 print(len(specs),'GLB;',sum(v['triangulos'] for v in MANIFEST),'triángulos')

if __name__=='__main__':main()
