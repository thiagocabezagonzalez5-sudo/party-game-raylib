"""Modelos GLB modulares para Pisotón de Plagas. Solo Python estándar."""
from geometria_glb import *
import geometria_glb as glb

glb.PALETTE.update({
 'verde_fondo':'#3e8542','verde_suelo':'#5eaa53','verde_luz':'#88c865','verde_oscuro':'#397748',
 'verde_tallo':'#3c995b','verde_oruga':'#81d452','verde_oruga_sombra':'#5dac48',
 'tierra':'#84553d','tierra_luz':'#ac7650','tierra_oscura':'#372b28',
 'madera':'#ad774b','madera_luz':'#d6a66d','madera_sombra':'#704d3e',
 'rosa':'#f26fa1','rosa_claro':'#ffafc9','amarillo':'#ffd764','violeta':'#a884df',
 'rojo_seta':'#dd5c55','crema':'#efe2c9','azul_regadera':'#598ac0','azul_luz':'#89b6d9',
 'gris':'#8b9294','gris_luz':'#b1b5ac','beetle':'#ad493e','beetle_luz':'#d46d4a',
 'negro':'#2c2d35','blanco':'#fff9e9','oro':'#ffd243','oro_sombra':'#eda835',
 'miel':'#ffc848','ala':'#c5e8ef','agua':'#c7efff',
 'COLOR_DINAMICO':'#ffffff','BOMBILLAS':'#fff0bb'})

def ground():
 m=mesh();box(m,(0,-.12,0),(90,.04,90),'verde_fondo')
 box(m,(0,-.55,0),(21,1,14.2),'tierra')
 box(m,(0,-.045,0),(21,.035,14.2),'verde_suelo')
 for k in range(20):
  x=-9.2+(k*43%185)/10;z=-5.6+(k*37%112)/10
  box(m,(x,-.025,z),(1.1+(k%4)*.34,.013,.7),'verde_luz' if k%2 else 'verde_oscuro')
 for k in range(54):
  x=-9.7+(k*71%196)/10;z=-6.4+(k*59%130)/10
  cylinder(m,(x,-.026,z),.047,.13+(k%3)*.05,'verde_luz',5,r_top=0)
 return m

def grass():
 m=mesh()
 for i in range(3):
  a=TAU*i/3;r=.08+(i%2)*.06
  beam(m,(r*math.cos(a),0,r*math.sin(a)),(r*math.cos(a)+.15*math.sin(a),3.4+(i%3)*.55,r*math.sin(a)),.065,'verde_oscuro' if i%2 else 'verde_tallo',5,r_end=.003)
 return m

def flower(color,height):
 m=mesh();beam(m,(0,0,0),(0,height,0),.13,'verde_tallo',7)
 for a in (1.3,3.5):
  x=.45*math.cos(a);z=.45*math.sin(a)
  beam(m,(0,height*.55,0),(x,height*.69,z),.07,'verde_tallo',6)
  ellipsoid(m,(x,height*.7,z),(.38,.055,.17),'verde_luz',10,4)
 for i in range(8):
  a=TAU*i/8
  ellipsoid(m,(.9*math.cos(a),height+.08,.9*math.sin(a)),(.45,.20,.45),color,11,5)
 ellipsoid(m,(0,height+.14,0),(.52,.32,.52),'amarillo',12,6)
 for i in range(9):
  a=TAU*i/9
  ellipsoid(m,(.32*math.cos(a),height+.445,.32*math.sin(a)),(.045,.025,.045),'tierra_luz',6,3)
 return m

def bud():
 m=mesh();beam(m,(0,0,0),(0,2.6,0),.1,'verde_tallo',7)
 for i in range(6):
  a=TAU*i/6;ellipsoid(m,(.19*math.cos(a),2.67,.19*math.sin(a)),(.2,.30,.20),'verde_luz',9,5)
 return m

def bloom():
 m=mesh();beam(m,(0,0,0),(0,2.6,0),.11,'verde_tallo',7)
 for i in range(8):
  a=TAU*i/8
  ellipsoid(m,(.75*math.cos(a),2.65,.75*math.sin(a)),(.43,.18,.43),'rosa' if i%2 else 'rosa_claro',12,5)
 ellipsoid(m,(0,2.72,0),(.43,.3,.43),'amarillo',12,6)
 return m

def flower_ring():
 m=mesh();torus(m,(0,.02,0),1.7,.042,'rosa',30,5)
 for i in range(8):
  a=TAU*i/8
  ellipsoid(m,(1.7*math.cos(a),.045,1.7*math.sin(a)),(.075,.035,.075),'rosa_claro',8,4)
 return m

def mushroom():
 m=mesh();cylinder(m,(0,0,0),.23,.68,'crema',9,r_top=.18)
 cylinder(m,(0,.65,0),.72,.12,'crema',12,r_top=.69)
 cylinder(m,(0,.71,0),.73,.58,'rojo_seta',12,r_top=0)
 for x,z in [(-.25,-.18),(.3,-.13),(.17,.32),(-.38,.12)]:
  ellipsoid(m,(x,1.02,z),(.085,.025,.08),'blanco',8,4)
 return m

def watering():
 m=mesh();cylinder(m,(0,0,0),.54,1.02,'azul_regadera',12,r_top=.59)
 torus(m,(0,1.03,0),.58,.045,'azul_luz',18,4)
 ellipsoid(m,(0,.95,0),(.48,.035,.48),'negro',12,4)
 beam(m,(.42,.73,0),(1.15,1.22,0),.11,'azul_regadera',8,r_end=.14)
 cylinder(m,(1.12,1.18,0),.26,.045,'azul_luz',12)
 beam(m,(-.45,.85,0),(-.95,1.15,0),.065,'azul_luz',7)
 beam(m,(-.95,1.15,0),(-.68,.29,0),.065,'azul_luz',7)
 return m

def rock():
 m=mesh();ellipsoid(m,(0,.21,0),(.56,.35,.48),'gris',9,5)
 ellipsoid(m,(-.18,.43,-.12),(.19,.06,.18),'gris_luz',7,3)
 return m

def fence(length,posts):
 m=mesh()
 for i in range(posts):
  x=-length/2+i*length/(posts-1)
  box(m,(x,.75,0),(.19,1.5,.17),'madera')
  cylinder(m,(x,1.5,0),.14,.19,'madera_luz',4,r_top=0)
  box(m,(x,.9,.10),(.045,.52,.025),'madera_sombra')
 for y in (.55,1.15):box(m,(0,y,.01),(length+.2,.16,.14),'madera_luz')
 return m

def burrow():
 m=mesh();cylinder(m,(0,-.045,0),.7,.14,'tierra_luz',14,r_top=.48)
 cylinder(m,(0,.09,0),.31,.014,'tierra_oscura',14)
 torus(m,(0,.103,0),.35,.035,'tierra',18,4)
 for i in range(8):
  a=TAU*i/8
  ellipsoid(m,(.6*math.cos(a),.02,.6*math.sin(a)),(.13,.045,.11),'tierra' if i%2 else 'tierra_luz',7,4)
 return m

def warning():
 m=mesh();torus(m,(0,.03,0),.85,.026,'COLOR_DINAMICO',28,4)
 return m

def beetle():
 m=mesh();ellipsoid(m,(-.08,.29,0),(.34,.30,.31),'beetle',14,7)
 ellipsoid(m,(-.08,.48,0),(.24,.085,.26),'beetle_luz',12,4)
 beam(m,(-.07,.57,-.2),(-.07,.57,.2),.018,'tierra_oscura',6)
 ellipsoid(m,(.3,.26,0),(.18,.18,.18),'negro',10,6)
 for sign in (-1,1):
  ellipsoid(m,(.31,.37,.12*sign),(.045,.047,.045),'blanco',8,4)
  for j in range(3):
   x=-.28+j*.2
   beam(m,(x,.22,sign*.22),(x-.1,.05,sign*.43),.027,'negro',6)
  beam(m,(.41,.35,.08*sign),(.63,.43,.16*sign),.025,'negro',6)
 return m

def caterpillar(front):
 m=mesh();ellipsoid(m,(0,.27,0),(.29,.29,.29),'verde_oruga' if front else 'verde_oruga_sombra',12,6)
 for sign in (-1,1):
  for x in (-.14,.15):ellipsoid(m,(x,.055,sign*.21),(.07,.06,.11),'verde_oscuro',7,4)
 if front:
  for sign in (-1,1):
   ellipsoid(m,(.23,.39,sign*.13),(.067,.075,.065),'blanco',9,5)
   ellipsoid(m,(.28,.39,sign*.13),(.028,.04,.026),'negro',7,4)
   beam(m,(0,.5,sign*.12),(.18,.75,sign*.15),.027,'verde_oscuro',6)
   ellipsoid(m,(.18,.75,sign*.15),(.055,.06,.055),'verde_luz',7,4)
 return m

def slug():
 m=mesh();ellipsoid(m,(-.18,.19,0),(.64,.20,.30),'oro_sombra',15,6)
 ellipsoid(m,(0,.31,0),(.35,.28,.27),'oro',14,7)
 ellipsoid(m,(.20,.46,0),(.17,.21,.18),'amarillo',11,6)
 for sign in (-1,1):
  beam(m,(.29,.48,sign*.12),(.45,.76,sign*.14),.035,'oro_sombra',7)
  ellipsoid(m,(.45,.77,sign*.14),(.065,.065,.06),'negro',8,4)
 ellipsoid(m,(-.08,.5,-.13),(.14,.035,.07),'BOMBILLAS',8,4)
 return m

def wasp():
 m=mesh();ellipsoid(m,(0,0,0),(.20,.17,.18),'negro',11,6)
 ellipsoid(m,(-.27,0,0),(.34,.19,.18),'miel',13,6)
 for x in (-.14,-.34,-.52):
  torus(m,(x,0,0),.17,.024,'negro',12,4,plane='yz')
 ellipsoid(m,(.24,0,0),(.15,.14,.14),'negro',10,5)
 for sign in (-1,1):
  ellipsoid(m,(.32,.06,sign*.085),(.045,.045,.04),'blanco',7,4)
  beam(m,(.32,.09,sign*.09),(.48,.20,sign*.16),.019,'negro',6)
 beam(m,(-.59,-.02,0),(-.76,-.10,0),.025,'negro',6,r_end=.003)
 return m

def wing():
 m=mesh();ellipsoid(m,(0,.025,.16),(.31,.023,.19),'ala',12,5)
 beam(m,(-.28,.048,.17),(.28,.048,.17),.008,'azul_luz',5)
 return m

def dewdrop():
 m=mesh();ellipsoid(m,(0,.09,0),(.10,.13,.10),'agua',10,6)
 ellipsoid(m,(-.025,.15,.055),(.025,.03,.016),'blanco',7,4)
 return m

def main():
 specs=[
 ('suelo_jardin',ground(),'origen global','Arena jugable X=±9, Z=±5.6; plano exterior 90×90.'),
 ('brizna_gigante',grass(),'pie central','34 instancias tras la cerca y los laterales; altura 3.4 a 4.2; escala Y si se desea.'),
 ('flor_fondo_rosa',flower('rosa',5.0),'pie central','Centro (-7,0,-9.4).'),
 ('flor_fondo_amarilla',flower('amarillo',5.6),'pie central','Centro (1.5,0,-10.6).'),
 ('flor_fondo_violeta',flower('violeta',6.2),'pie central','Centro (8,0,-9.4).'),
 ('flor_brote',bud(),'pie central','Flor activa, aviso previo: escalar tallo según crecimiento.'),
 ('flor_abierta',bloom(),'pie central','Flor activa completamente abierta; instanciar en (florX,0,florZ).'),
 ('aro_flor',flower_ring(),'centro','Radio lógico 1.7; en (florX,0,florZ), sólo durante floración.'),
 ('seta',mushroom(),'pie central','Tres posiciones exteriores: (-10.8,3.8), (10.9,2.6), (10.6,-1.2).'),
 ('regadera',watering(),'pie central','Posición (-11.2,0,-3).'),
 ('piedra',rock(),'pie central','Cinco posiciones exteriores; rotar y variar escala para diversidad.'),
 ('cerca_fondo',fence(22,21),'pie central','Posición (0,0,-6.9).'),
 ('cerca_lateral',fence(11.5,11),'pie central','Dos copias X=±10.4, rotar Y=90°.'),
 ('madriguera',burrow(),'pie central','Seis centros X=(-7.2,0,7.2), Z=(-4.3/-4.7,4.3/4.7).'),
 ('aro_aviso',warning(),'centro','Durante aviso de madriguera, radio .85 y tinte naranja.'),
 ('escarabajo',beetle(),'pie central','Orientar +X a dirección de marcha; escala según aparición y aturdimiento.'),
 ('oruga_cabeza',caterpillar(True),'pie central','Primer segmento vivo; orientar +X a dirección; valor 2 por segmento.'),
 ('oruga_segmento',caterpillar(False),'pie central','Instanciar por cada segmento vivo en segX[s],segZ[s]; separación lógica .55.'),
 ('babosa_dorada',slug(),'pie central','Orientar +X al avance; valor 5; brillo estático de referencia.'),
 ('avispa_cuerpo',wasp(),'centro de cuerpo','Instanciar en (x,altura,z), altura≈.95; orientar +X a dirección.'),
 ('avispa_ala',wing(),'unión al tórax','Dos instancias ±Z=.22 respecto a cuerpo, animar Y con sin(t*55)*.12.'),
 ('gota_rocio',dewdrop(),'pie central','Diez instancias en arena, decorativas.'),
 ]
 for name,m,pivot,usage in specs:save(name,m,pivot,usage)
 (ROOT/'manifest.json').write_text(json.dumps({'minijuego':'Pisotón de Plagas','base':'claude/expansion-party ea5472c','coordenadas':'Y arriba; 1 unidad GLB = 1 unidad raylib','modelos':MANIFEST},ensure_ascii=False,indent=2))
 print(len(specs),'GLB;',sum(v['triangulos'] for v in MANIFEST),'triángulos')

if __name__=='__main__':main()
