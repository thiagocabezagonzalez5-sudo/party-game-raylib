"""Modelos originales GLB 2.0 para Descenso en Nubes."""
from geometria_glb import *
import geometria_glb as glb

glb.PALETTE.update({
 'cielo':'#7cc6ee','cielo_claro':'#b9e6fa','blanco':'#f8fbf5',
 'nube':'#e8f2fa','nube_sombra':'#c5dbe9','azul':'#45a2dd',
 'azul_oscuro':'#295a8c','turquesa':'#79d7da','metal':'#9aaebd',
 'madera':'#9b6c4a','madera_oscura':'#674c3d','hierba':'#6ebd69',
 'hierba_clara':'#a7d87d','roca':'#977964','roca_clara':'#c19b7b',
 'oro':'#efc34f','oro_claro':'#fff0a3','rojo':'#e8645f',
 'rojo_claro':'#f8aaa1','tormenta':'#5e6682','tormenta_clara':'#8993aa',
 'tormenta_oscura':'#404963','rayo':'#ffe575','arco_naranja':'#f08a50',
 'arco_amarillo':'#f0d66d','arco_verde':'#83c884','arco_azul':'#69b9dc',
 'arco_anil':'#617aba','arco_violeta':'#a483c4',
 'COLOR_DINAMICO':'#ffffff','BOMBILLAS':'#fff9d2',
})

def doble(m,a,b,c,d,mat):
 quad(m,a,b,c,d,mat);quad(m,d,c,b,a,mat)

def planeador(freno=False):
 m=mesh();w=3.4 if freno else 2.2
 # Origen en el centro del jugador: ala principal Y=1.3.
 a=(-w/2,1.31,.47);b=(-.25,1.39,-.58);c=(.25,1.39,-.58);d=(w/2,1.31,.47)
 doble(m,a,b,c,(-.0,1.3,.46),'COLOR_DINAMICO')
 doble(m,(0,1.3,.46),c,d,a,'COLOR_DINAMICO')
 # Refuerzo claro en zigzag y punta azul, visible mirando desde arriba.
 for side in (-1,1):
  x=side*w*.47
  beam(m,(0,1.35,-.5),(x,1.32,.4),.033,'azul_oscuro',6)
  beam(m,(0,.50,0),(x,1.29,.32),.021,'metal',6)
  box(m,(x,1.30,.42),(.18,.04,.20),'rojo' if freno else 'azul')
 beam(m,(0,1.39,-.56),(0,1.35,.50),.038,'azul_oscuro',6)
 ellipsoid(m,(0,1.40,-.15),(.17,.035,.22),'oro',8,4)
 box(m,(0,.52,.02),(.10,.18,.12),'madera_oscura')
 return m

def anillo(dorado=False):
 m=mesh();r=1.0 if dorado else .85
 torus(m,(0,0,0),r,.075,'oro' if dorado else 'blanco',28,6)
 torus(m,(0,.035,0),r-.11,.024,'oro_claro' if dorado else 'azul_oscuro',28,5)
 for i in range(6):
  a=i*TAU/6
  ellipsoid(m,(r*math.cos(a),.05,r*math.sin(a)),(.07,.035,.07),'oro_claro' if dorado else 'turquesa',8,4)
 if dorado:
  ellipsoid(m,(0,.045,0),(.19,.065,.19),'oro_claro',10,5)
  torus(m,(0,.12,0),.23,.022,'BOMBILLAS',16,4)
 return m

def estrella():
 m=mesh();star(m,(0,0,0),.29,'oro',thick=.08)
 ellipsoid(m,(0,0,.08),(.07,.08,.03),'oro_claro',8,5)
 return m

def nube():
 m=mesh()
 for p,s,mat in [((0,0,0),(1.12,.64,.80),'nube'),
                 ((-.90,-.20,.12),(.82,.52,.70),'nube_sombra'),
                 ((.95,-.14,-.08),(.91,.53,.69),'nube'),
                 ((-.35,.35,-.03),(.72,.53,.62),'blanco'),
                 ((.45,.30,-.24),(.69,.54,.57),'blanco')]:
  ellipsoid(m,p,s,mat,12,6)
 return m

def tormenta():
 m=mesh()
 for p,s,mat in [((0,0,0),(1.22,.80,1.0),'tormenta_oscura'),
                 ((1.04,-.12,0),(.94,.62,.80),'tormenta'),
                 ((-1.00,-.18,.10),(.86,.61,.78),'tormenta'),
                 ((-.25,.36,-.12),(.89,.56,.72),'tormenta_clara')]:
  ellipsoid(m,p,s,mat,12,6)
 beam(m,(-.04,-.53,.25),(.25,-1.02,.29),.12,'rayo',5)
 beam(m,(.25,-1.02,.29),(-.18,-1.50,.33),.11,'rayo',5,r_end=.015)
 torus(m,(0,-.16,0),1.92,.036,'rojo',28,4)
 return m

def flecha_viento():
 m=mesh()
 beam(m,(-.83,0,0),(.36,0,0),.065,'blanco',7)
 beam(m,(.30,0,0),(1.14,0,0),.27,'turquesa',8,r_end=0)
 for z in (-.19,.19):
  beam(m,(-.78,.03,z),(-.2,.03,z),.018,'azul_oscuro',5)
 return m

def banda_viento():
 m=mesh()
 for y in (-1.6,1.6):
  torus(m,(0,y,0),6,.042,'cielo_claro',40,5)
 for i in range(12):
  a=TAU*i/12
  x,z=6*math.cos(a),6*math.sin(a)
  beam(m,(x,-1.58,z),(x,1.58,z),.017,'turquesa',5)
 return m

def mar_nubes():
 m=mesh();box(m,(0,-2.62,0),(80,.20,80),'nube')
 for x in range(-36,37,12):
  for z in range(-36,37,12):
   if (x*3+z*5)%7<4:
    ellipsoid(m,(x,-2.45,z),(3,.18,1.5),'blanco',10,4)
 return m

def isla_principal():
 m=mesh()
 cylinder(m,(0,-5.5,0),1.0,5.22,'roca',20,r_top=6.95)
 for a in range(16):
  t=a*TAU/16
  beam(m,(.75*math.cos(t),-5.3,.75*math.sin(t)),
       (6.4*math.cos(t),-.38,6.4*math.sin(t)),.037,'roca_clara',5)
 cylinder(m,(0,-.28,0),7,.28,'hierba',28)
 torus(m,(0,-.03,0),6.88,.09,'hierba_clara',28,5)
 for i in range(16):
  a=TAU*i/16
  x,z=5.4*math.cos(a),5.4*math.sin(a)
  ellipsoid(m,(x,.10,z),(.25,.11,.18),'hierba_clara',8,4)
 return m

def diana():
 m=mesh()
 cylinder(m,(0,.013,0),1.80,.018,'rojo',32)
 cylinder(m,(0,.033,0),1.19,.018,'blanco',32)
 cylinder(m,(0,.053,0),.59,.018,'rojo',32)
 torus(m,(0,.072,0),.23,.015,'oro_claro',20,4)
 return m

def isla_flotante():
 m=mesh()
 cylinder(m,(0,-2.6,0),.43,2.31,'roca',12,r_top=2.4)
 cylinder(m,(0,-.3,0),2.4,.3,'hierba',14)
 torus(m,(0,-.01,0),2.38,.05,'hierba_clara',14,5)
 cylinder(m,(.45,0,-.24),.11,.95,'madera',7,r_top=.08)
 ellipsoid(m,(.45,1.23,-.24),(.73,.63,.68),'hierba',10,5)
 ellipsoid(m,(.75,1.26,-.13),(.39,.43,.40),'hierba_clara',9,5)
 return m

def globo(rojo=False):
 m=mesh();principal='rojo' if rojo else 'azul';sec='oro' if rojo else 'turquesa'
 # Pivote en centro de la envolvente, canasta debajo del origen.
 ellipsoid(m,(0,0,0),(1.58,1.82,1.55),principal,20,10)
 for i in range(8):
  a=TAU*i/8;x,z=1.59*math.cos(a),1.56*math.sin(a)
  beam(m,(x*.5,1.53,z*.5),(x,-.34,z),.025,sec,5)
 cylinder(m,(0,-2.74,0),.45,.50,'madera',10,r_top=.53)
 torus(m,(0,-2.23,0),.50,.055,'madera_oscura',12,5)
 for a in (0,TAU/4,TAU/2,3*TAU/4):
  beam(m,(.36*math.cos(a),-2.22,.36*math.sin(a)),
       (.78*math.cos(a),-1.16,.78*math.sin(a)),.025,'madera_oscura',5)
 cylinder(m,(0,-2.09,0),.22,.24,'oro',8,r_top=.10)
 return m

def molino():
 m=mesh()
 cylinder(m,(0,0,0),.8,3.9,'blanco',8,r_top=.49)
 cylinder(m,(0,3.91,0),.73,.85,'rojo',8,r_top=0)
 box(m,(0,.92,.77),(.55,1.26,.12),'madera_oscura')
 box(m,(0,2.37,.52),(.47,.48,.09),'azul_oscuro')
 beam(m,(0,4.15,.38),(0,4.15,.88),.17,'madera_oscura',10)
 return m

def aspas():
 m=mesh()
 # Pivote en eje, plano XY; motor del juego aplica giro alrededor de Z.
 ellipsoid(m,(0,0,0),(.22,.22,.17),'oro',12,6)
 for i in range(4):
  a=i*TAU/4
  u=(math.cos(a),math.sin(a));v=(-math.sin(a),math.cos(a))
  def pt(r,off,z):return (u[0]*r+v[0]*off,u[1]*r+v[1]*off,z)
  beam(m,pt(.14,0,0),pt(2.16,0,0),.055,'madera',6)
  doble(m,pt(.38,.02,.04),pt(1.98,.02,.04),pt(1.70,.48,.04),pt(.66,.33,.04),'blanco')
  beam(m,pt(.65,.33,.06),pt(1.7,.48,.06),.02,'azul',5)
 return m

def ave():
 m=mesh()
 ellipsoid(m,(0,0,0),(.22,.12,.34),'azul_oscuro',10,5)
 for s in (-1,1):
  beam(m,(s*.14,.05,0),(s*.62,.2,.08),.06,'azul_oscuro',6,r_end=.025)
  beam(m,(s*.62,.2,.08),(s*.95,.02,-.06),.045,'azul',6,r_end=.012)
 ellipsoid(m,(0,.03,-.27),(.12,.09,.14),'blanco',8,4)
 beam(m,(0,.01,-.37),(0,.02,-.53),.04,'oro',5,r_end=0)
 return m

def arcoiris():
 m=mesh()
 # Origen mundial de referencia (0,62,-16).
 mats=['rojo','arco_naranja','arco_amarillo','arco_verde',
       'arco_azul','arco_anil','arco_violeta']
 for band,mat in enumerate(mats):
  r=13+band*.50
  for s in range(32):
   a=math.pi*s/32;b=math.pi*(s+1)/32
   beam(m,(r*math.cos(a),-10+r*.8*math.sin(a),0),
        (r*math.cos(b),-10+r*.8*math.sin(b),0),.22,mat,5)
 return m

def main():
 specs=[
  ('planeador',planeador(),'centro del jugador','Jugador.position; ala Y=+1.3; tintar COLOR_DINAMICO con color de equipo.'),
  ('planeador_frenado',planeador(True),'centro del jugador','Jugador.position mientras frenando>0; ancho 3.4.'),
  ('anillo_blanco',anillo(),'centro de anillo','anillo.posicion; radio .85 y valor 1.'),
  ('anillo_dorado',anillo(True),'centro de anillo','anillo.posicion; radio 1 y valor 3.'),
  ('estrella',estrella(),'centro de estrella','Decoración o efecto de recogida, sin colisión.'),
  ('nube_blanca',nube(),'centro de nube','Alrededor de cilindro de caída a radio 8.5..15.5.'),
  ('nube_tormenta',tormenta(),'centro de tormenta','tormenta.posicion; escalar por radio/1.6 si cambia.'),
  ('flecha_viento',flecha_viento(),'centro de flecha','En viento.altura; eje +X al sentido dirX,dirZ.'),
  ('banda_viento',banda_viento(),'centro de banda','(0,viento.altura,0); radio 6, altura ±1.6.'),
  ('mar_de_nubes',mar_nubes(),'centro del plano','(0,0,0); superficie Y=-2.52 cerca de la isla.'),
  ('isla_principal',isla_principal(),'centro en la superficie Y=0','(0,0,0); radio 7 y base hasta Y=-5.5.'),
  ('diana_aterrizaje',diana(),'centro del blanco','(0,0,0); radio de bonus 1.8.'),
  ('isla_flotante',isla_flotante(),'centro de cubierta','A radio 10.5..13.5 y altura 12+13*k.'),
  ('globo_azul',globo(),'centro de la envolvente','En torno a X=±9.5, Y=20+17*k; mover suavemente.'),
  ('globo_rojo',globo(True),'centro de la envolvente','Variante de globo para repetir en altura.'),
  ('molino_torre',molino(),'base del molino','Isla principal (-5.2,0,-3) o islas lejanas.'),
  ('aspas_molino',aspas(),'eje de aspas','Molino (x,4.15,z+.88); rotar sobre eje Z.'),
  ('ave',ave(),'centro del cuerpo','A radio ~8, alturas 6+12*k; animar posición/aleteo.'),
  ('arcoiris',arcoiris(),'centro nominal del arco','(0,62,-16), visible cerca de altura 62.'),
 ]
 for name,model,pivot,usage in specs:save(name,model,pivot,usage)
 (glb.ROOT/'manifest.json').write_text(json.dumps({
  'version':1,'rama_referencia':'claude/expansion-party','commit_referencia':'ea5472c',
  'unidades':'1 unidad GLB = 1 unidad raylib','ejes':'Y arriba; caída desde Y=100 hacia Y=0',
  'licencia':'Arte original procedural sin recursos externos',
  'nota':'Física, recogida, ráfagas, colisiones, frenado y puntaje permanecen en C++.',
  'modelos':glb.MANIFEST},ensure_ascii=False,indent=2),encoding='utf8')
 print(len(glb.MANIFEST),'GLB;',sum(x['triangulos'] for x in glb.MANIFEST),'triangulos')

if __name__=='__main__':main()
