"""GLB procedurales originales para Bolas de Azúcar. Python 3 estándar."""
from geometria_glb import *
import geometria_glb as glb

glb.PALETTE.update({'rosa_fondo':'#efb4cf','rosa_claro':'#f9d1e0','galleta':'#cc955d','galleta_luz':'#e1b57f','galleta_sombra':'#a76d48','pepita':'#5e382e','pepita_luz':'#966149','glaseado':'#fff4f8','glaseado_sombra':'#f4d8e6','azucar':'#f8fbff','azucar_sombra':'#e3e9f8','chocolate':'#4f2b25','chocolate_luz':'#805039','chocolate_espuma':'#b37a50','gom_roja':'#f55372','gom_verde':'#5ddc84','gom_amarilla':'#ffc95b','gom_azul':'#54a5f9','gom_brillo':'#ffffff','pir_rosa':'#ef5999','pir_azul':'#5ec8f1','pir_amarilla':'#f6cc5b','pir_violeta':'#aa83ec','pir_verde':'#80ddb0','palito':'#f3eee0','caramelo':'#eb9c53','caramelo_oscuro':'#bf6a5b','caramelo_claro':'#ffdf9d','nata':'#fffaf4','nata_sombra':'#ded6dc','cereza':'#d7395d','cereza_brillo':'#ff8f9a','bola':'#fffaff','bola_sombra':'#e2e5f5','COLOR_DINAMICO':'#ffffff'})

def both(m,a,b,c,d,mat):quad(m,a,b,c,d,mat);quad(m,d,c,b,a,mat)
def backdrop():
 m=mesh();box(m,(0,-.575,0),(140,.05,140),'rosa_fondo')
 for i in range(80):
  x=(i*37%129)-64;z=(i*53%129)-64
  if abs(x)<12 and abs(z)<12:continue
  ellipsoid(m,(x,-.535,z),(.3,.015,.20),'rosa_claro',7,3)
 return m

def cookie():
 m=mesh();box(m,(0,-.25,0),(18,.5,18),'galleta')
 for x in range(-8,9,2):
  box(m,(x,-.03,0),(.026,.015,17.9),'galleta_sombra')
 for z in range(-8,9,2):
  box(m,(0,-.025,z),(17.9,.015,.026),'galleta_sombra')
 sugar=[(-8,-4,-4.5,4.5),(4,8,-4.5,4.5),(-3,3,-8,-5.5),(-3,3,5.5,8)]
 puddles=[(0,0,2.2),(-5.5,-6.5,1.4),(5.5,6.5,1.4)]
 for i in range(28):
  x=-8.4+((i*37)%168)/10;z=-8.4+((i*53)%168)/10
  if any(a<=x<=b and c<=z<=d for a,b,c,d in sugar):continue
  if any((x-a)**2+(z-b)**2<=r*r for a,b,r in puddles):continue
  cylinder(m,(x,-.001,z),.15,.036,'pepita' if i%3 else 'pepita_luz',8)
 for x,z in [(-8.6,-8.6),(8.6,-8.6),(-8.6,8.6),(8.6,8.6)]:
  ellipsoid(m,(x,.008,z),(.2,.06,.2),'galleta_luz',8,4)
 return m

def sugar_patch(w,d):
 m=mesh();box(m,(0,.027,0),(w,.055,d),'azucar')
 for i in range(26):
  x=(((i*17)%101)/100-.5)*(w-.3);z=(((i*29)%103)/102-.5)*(d-.3)
  ellipsoid(m,(x,.063,z),(.045,.023,.05),'azucar_sombra' if i%4 else 'glaseado',6,3)
 return m

def puddle(r):
 m=mesh();cylinder(m,(0,.005,0),r,.055,'chocolate',24,r_top=r*.98)
 cylinder(m,(0,.062,0),r*.55,.014,'chocolate_luz',22)
 for i in range(9):
  a=TAU*i/9;rr=r*(.72+.08*(i%3));ellipsoid(m,(rr*math.cos(a),.066,rr*math.sin(a)),(.12,.025,.09),'chocolate_espuma',8,4)
 torus(m,(0,.06,0),r*.98,.035,'chocolate_luz',24,4)
 return m

def cookie_wall():
 m=mesh();box(m,(0,1,0),(18.8,2,.4),'galleta_sombra')
 box(m,(0,2.055,0),(18.87,.12,.48),'glaseado_sombra')
 for i in range(19):
  x=-8.9+i*.99
  ellipsoid(m,(x,2.12,0),(.2,.055,.24),'glaseado',9,4)
  if i%3==0:ellipsoid(m,(x,1.24,.22),(.10,.06,.04),'pepita',8,4)
 return m

def gum(i):
 m=mesh();colors=['gom_roja','gom_verde','gom_amarilla','gom_azul'];c=colors[i]
 cylinder(m,(0,0,0),.8,1.02,c,16,r_top=.79)
 ellipsoid(m,(0,1.0,0),(.8,.79,.8),c,16,7)
 ellipsoid(m,(-.27,1.35,.58),(.16,.20,.075),'gom_brillo',9,5)
 ellipsoid(m,(.14,1.53,.5),(.09,.12,.05),'gom_brillo',8,4)
 torus(m,(0,.12,0),.79,.028,'glaseado',22,4)
 return m

def lollipop(i):
 m=mesh();colors=['pir_rosa','pir_azul','pir_amarilla','pir_violeta','pir_verde'];c=colors[i]
 cylinder(m,(0,0,0),.12,3.62,'palito',10)
 ellipsoid(m,(0,4.4,0),(1.2,1.2,.63),c,24,9)
 torus(m,(0,4.4,.52),.91,.11,'glaseado',26,5,plane='xy')
 torus(m,(0,4.4,.625),.48,.065,'glaseado_sombra',25,5,plane='xy')
 ellipsoid(m,(-.42,4.85,.56),(.28,.15,.035),'glaseado',9,4)
 for y in (.5,1.7,2.9):torus(m,(0,y,0),.12,.018,'glaseado_sombra',12,4)
 return m

def caramel():
 m=mesh();cylinder(m,(0,0,0),.9,6.0,'caramelo',12,r_top=.8)
 # Helice de fondant alrededor del fuste.
 for j in range(46):
  t0=j/46;t1=(j+1)/46
  a=TAU*3*t0;b=TAU*3*t1
  p=(.88*math.cos(a),6*t0,.88*math.sin(a));q=(.88*math.cos(b),6*t1,.88*math.sin(b))
  beam(m,p,q,.095,'caramelo_claro' if j%2 else 'caramelo_oscuro',5)
 cylinder(m,(0,6,0),1.1,.32,'caramelo_claro',12,r_top=1.03)
 for k in range(12):
  a=TAU*k/12
  ellipsoid(m,(.96*math.cos(a),6.12,.96*math.sin(a)),(.14,.1,.14),'glaseado',7,4)
 return m

def cream_mountain():
 m=mesh();cylinder(m,(0,0,0),4.3,3.2,'galleta_sombra',10,r_top=4.0)
 cylinder(m,(0,3.2,0),4.2,5.0,'nata',10,r_top=.4)
 for i in range(10):
  a=TAU*i/10
  beam(m,(3.8*math.cos(a),4.0,3.8*math.sin(a)),(.4*math.cos(a),8.7,.4*math.sin(a)),.055,'nata_sombra',5)
 ellipsoid(m,(0,8.5,0),(.75,.75,.75),'cereza',12,6)
 ellipsoid(m,(-.25,8.8,.4),(.18,.13,.075),'cereza_brillo',9,4)
 beam(m,(0,9.0,0),(.13,9.45,0),.038,'galleta_sombra',6)
 return m

def snowball():
 m=mesh();ellipsoid(m,(0,0,0),(1,1,1),'bola',18,9)
 for i in range(13):
  a=TAU*i/13;y=-.85+1.7*((i*5)%13)/12;rr=math.sqrt(max(0,1-y*y));x=rr*math.cos(a);z=rr*math.sin(a)
  ellipsoid(m,(x*.95,y*.95,z*.95),(.042,.045,.042),'bola_sombra',7,4)
 torus(m,(0,0,0),.99,.027,'COLOR_DINAMICO',28,4)
 return m

def chip():
 m=mesh();ellipsoid(m,(0,.06,0),(.19,.08,.15),'pepita',9,5);return m

def main():
 specs=[('fondo_rosa',backdrop(),'origen global','Plano decorativo 140x140, Y=-.55.'),('suelo_galleta',cookie(),'centro del suelo','Arena 18x18, Y superior 0.'),('azucar_lateral',sugar_patch(4,9),'centro','Centros X=±6, Z=0; zonas 4x9.'),('azucar_extremo',sugar_patch(6,2.5),'centro','Centros X=0, Z=±6.75.'),('chocolate_grande',puddle(2.2),'centro','Charco en (0,0,0), radio lógico 2.2.'),('chocolate_pequeno',puddle(1.4),'centro','Dos en (-5.5,0,-6.5), (5.5,0,6.5).'),('muro_galleta',cookie_wall(),'centro al pie','Cuatro muros, X/Z=±9.2; usar giro Y=90° para lados.'),]
 for i,name in enumerate(['roja','verde','amarilla','azul']):specs.append(('gominola_'+name,gum(i),'pie central',f'Gominola {i}, radio 0.8; centros X/Z=±3.2.'))
 for i,name in enumerate(['rosa','azul','amarilla','violeta','verde']):specs.append(('piruleta_'+name,lollipop(i),'pie central',f'Árbol de piruleta color {name}, altura visual ≈5.6.'))
 specs += [('columna_caramelo',caramel(),'pie central','Cuatro fuera de arena, X=±12.5 Z=±12.'),('montana_nata',cream_mountain(),'pie central','Tres al fondo, X=-22,-2,18 y Z=-30.'),('bola_azucar',snowball(),'centro','Escalar uniformemente a radio lógico entre .3 y 1.4; tintar aro.'),('pepita_chocolate',chip(),'pie central','Decorado modular para suelo.')]
 for name,m,pivot,usage in specs:save(name,m,pivot,usage)
 (ROOT/'manifest.json').write_text(json.dumps({'minijuego':'Bolas de Azúcar','base':'claude/expansion-party ea5472c','arena_lado':18,'radio_gominola':.8,'radio_bola_min':.3,'radio_bola_max':1.4,'modelos':MANIFEST},ensure_ascii=False,indent=2))
 print(len(specs),'GLB;',sum(v['triangulos'] for v in MANIFEST),'triángulos')
if __name__=='__main__':main()
