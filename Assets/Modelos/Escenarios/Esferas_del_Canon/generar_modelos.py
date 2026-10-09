"""Arte GLB procedural original para Esferas del Cañón; Python 3 estándar."""
from geometria_glb import *
import geometria_glb as glb

glb.PALETTE.update({'lecho':'#bd7954','lecho_alt':'#c9845b','arena':'#e6c687','arena_luz':'#f0daa5','grieta':'#271d29','borde':'#6a3c35','estrato_rojo':'#8c493e','estrato_cobre':'#bc7052','estrato_crema':'#d9a477','arenisca':'#c98156','arenisca_clara':'#dfa574','arenisca_oscura':'#98533e','madera':'#996842','madera_clara':'#c08b55','madera_oscura':'#5d4137','cuerda':'#ddbb83','verde':'#448b61','verde_claro':'#62aa70','espina':'#f2d497','roca':'#945f52','roca_clara':'#b47b5f','roca_oscura':'#5f4141','blanco':'#f5eee0','negro':'#33313a','meta':'#df6257','salida':'#5bc396','oro':'#f4ce71','esfera':'#998777','esfera_clara':'#b4a296','esfera_oscura':'#615952','bandera':'#57bb86','COLOR_DINAMICO':'#ffffff'})
CONTROL=[0,0,3,8,9,4,-3,-8,-9,-4,3,7,3,0]
P=[]
for i in range(13):
 a=max(0,i-1);d=min(13,i+2)
 for k in range(4):
  t=k/4;t2=t*t;t3=t2*t
  x=.5*(2*CONTROL[i]+(-CONTROL[a]+CONTROL[i+1])*t+(2*CONTROL[a]-5*CONTROL[i]+4*CONTROL[i+1]-CONTROL[d])*t2+(-CONTROL[a]+3*CONTROL[i]-3*CONTROL[i+1]+CONTROL[d])*t3)
  P.append((x,-10*(i+t)))
P.append((CONTROL[-1],-130))
S=[0]
for (ax,az),(bx,bz) in zip(P,P[1:]):S.append(S[-1]+math.hypot(bx-ax,bz-az))
GAPS=[(40,43.5,1.5,1),(74,77.5,-1.5,1),(96,102,0,.8)]
SAND=[(18,26),(64,71),(108,116)]

def point(s,l=0,h=0):
 s=max(0,min(S[-1],s));i=0
 while i<len(S)-2 and S[i+1]<s:i+=1
 x0,z0=P[i];x1,z1=P[i+1];length=math.hypot(x1-x0,z1-z0)
 tx=(x1-x0)/length;tz=(z1-z0)/length;t=(s-S[i])/length
 return (x0+(x1-x0)*t-tz*l,-.12*s+h,z0+(z1-z0)*t+tx*l)

def both(m,a,b,c,d,mat):quad(m,a,b,c,d,mat);quad(m,d,c,b,a,mat)
def strip(m,s0,s1,l0,l1,h,mat,step=.8):
 n=max(1,math.ceil((s1-s0)/step))
 for i in range(n):
  a=s0+(s1-s0)*i/n;b=s0+(s1-s0)*(i+1)/n
  both(m,point(a,l0,h),point(a,l1,h),point(b,l1,h),point(b,l0,h),mat)

def track(a,b):
 m=mesh();stops=[a,b]+[x for p in GAPS+SAND for x in p[:2] if a<x<b]+[v for v in S if a<v<b]
 stops=sorted(set(stops))
 for lo,hi in zip(stops,stops[1:]):
  mid=(lo+hi)/2;gap=any(g0<=mid<=g1 for g0,g1,_,_ in GAPS);sand=any(s0<=mid<=s1 for s0,s1 in SAND)
  strip(m,lo,hi,-4.2,4.2,0,'grieta' if gap else ('lecho' if int(mid/2)%2 else 'lecho_alt'),hi-lo)
  if gap:
   strip(m,lo,hi,-4.2,-3.92,.045,'borde',hi-lo);strip(m,lo,hi,3.92,4.2,.045,'borde',hi-lo)
  if sand:
   strip(m,lo,hi,-4,4,.045,'arena',hi-lo)
   for l in (-2.8,0,2.8):ellipsoid(m,point(mid,l,.06),(.17,.02,.11),'arena_luz',6,3)
 return m

def wall(a,b,side):
 m=mesh();stops=sorted(set([a,b]+[v for v in S if a<v<b]))
 for lo,hi in zip(stops,stops[1:]):
  for bottom,top,mat in [(-.3,0,'estrato_rojo'),(0,2.4,'estrato_cobre'),(2.4,4.4,'estrato_crema'),(4.4,7,'arenisca')]:
   both(m,point(lo,side*4.4,bottom),point(hi,side*4.4,bottom),point(hi,side*4.4,top),point(lo,side*4.4,top),mat)
  both(m,point(lo,side*4.4,7),point(hi,side*4.4,7),point(hi,side*5.25,7.1),point(lo,side*5.25,7.1),'arenisca_clara')
  both(m,point(lo,side*5.25,-.3),point(hi,side*5.25,-.3),point(hi,side*5.25,7.1),point(lo,side*5.25,7.1),'arenisca_oscura')
 return m

def bridge(g):
 m=mesh();s0,s1,c,w=GAPS[g];n=math.ceil((s1-s0)/.65)
 for i in range(n):
  a=s0+(s1-s0)*i/n;b=s0+(s1-s0)*(i+1)/n-.06
  strip(m,a,b,c-w,c+w,.09,'madera_clara',b-a)
  for l in (c-w+.1,c+w-.1):beam(m,point(a,l,.1),point(b,l,.1),.025,'madera_oscura',5)
 for side in (-4.2,4.2):
  for s in (s0-.2,s1+.2):
   x,y,z=point(s,side,0);beam(m,(x,y,z),(x,y+1.05,z),.11,'madera_oscura',6)
  beam(m,point(s0-.2,side,1.05),point(s1+.2,side,1.05),.025,'cuerda',6)
 for l in (-3,3):
  for s in (s0-.55,s1+.15):strip(m,s,s+.35,l-.3,l+.3,.1,'madera_oscura',.35)
 return m

def ramp():
 m=mesh();s0,s1=90,93.5
 for i in range(8):
  a=s0+(s1-s0)*i/8;b=s0+(s1-s0)*(i+1)/8
  ha=.05+.95*(a-s0)/(s1-s0);hb=.05+.95*(b-s0)/(s1-s0)
  both(m,point(a,2,ha),point(a,3.6,ha),point(b,3.6,hb),point(b,2,hb),'madera_clara')
 for l in (2.04,3.56):beam(m,point(s0,l,.12),point(s1,l,1.07),.045,'oro',6)
 return m

def rock_arch():
 m=mesh();s=12
 for side in (-4.8,4.8):
  x,y,z=point(s,side,0);cylinder(m,(x,y-.05,z),1.12,5.7,'arenisca_oscura',7,r_top=.87)
  ellipsoid(m,(x,y+3.4,z),(.95,.72,.85),'estrato_cobre',7,4)
 beam(m,point(s,-4.8,5.9),point(s,4.8,5.9),.72,'arenisca',9)
 for l in (-2.8,0,2.8):
  x,y,z=point(s,l,6.45);ellipsoid(m,(x,y,z),(.9,.55,.8),'arenisca_clara',8,4)
 return m

def mesa():
 m=mesh();box(m,(0,5,0),(6.7,10,5.7),'estrato_cobre');box(m,(0,10.15,0),(7.7,.75,6.5),'arenisca_clara')
 for y,mat in [(2,'estrato_rojo'),(4,'estrato_crema'),(7,'arenisca_oscura')]:box(m,(0,y,2.86),(6.74,.13,.08),mat)
 return m

def cactus():
 m=mesh();cylinder(m,(0,0,0),.31,1.7,'verde',8,r_top=.24);ellipsoid(m,(0,1.7,0),(.24,.27,.24),'verde_claro',10,5)
 for side,y,h in [(-1,.75,.75),(1,1.02,.56)]:
  beam(m,(side*.15,y,0),(side*.55,y,0),.15,'verde',8)
  beam(m,(side*.55,y,0),(side*.55,y+h,0),.15,'verde_claro',8)
 for y in (.3,.8,1.3):
  for j in range(8):
   a=TAU*j/8;beam(m,(.28*math.cos(a),y,.28*math.sin(a)),(.38*math.cos(a),y+.07,.38*math.sin(a)),.013,'espina',4)
 return m

def fallen_rock():
 m=mesh();ellipsoid(m,(0,.52,0),(.92,.69,.8),'roca',9,5)
 for p in [(-.41,.89,.55),(.32,.62,.69)]:ellipsoid(m,p,(.38,.1,.09),'roca_clara',6,3)
 return m

def checkpoint():
 m=mesh();beam(m,(0,0,0),(0,3.15,0),.085,'madera_oscura',8)
 both(m,(0,2.65,.04),(.92,2.65,.04),(.7,3.07,.04),(0,3.07,.04),'bandera')
 ellipsoid(m,(0,3.2,0),(.13,.13,.13),'oro',8,4);cylinder(m,(0,0,0),.27,.12,'arenisca',8)
 return m

def gate(finish):
 m=mesh()
 for x in (-4,4):
  box(m,(x,2.35,0),(.62,4.7,.64),'arenisca_clara');box(m,(x,4.8,0),(.84,.36,.84),'arenisca')
 box(m,(0,4.72,0),(8.75,.56,.55),'meta' if finish else 'salida')
 for i in range(8):box(m,(-3.9+i*1.11,.05,.14),(1.09,.08,.85),'blanco' if i%2 else 'negro')
 for x in (-3.4,3.4):ellipsoid(m,(x,5.1,0),(.18,.18,.18),'oro',8,4)
 return m

def sphere():
 m=mesh();ellipsoid(m,(0,0,0),(1,1,1),'esfera',16,9)
 for v in [(1,0,0),(-1,0,0),(0,1,0),(0,-1,0),(0,0,1),(0,0,-1)]:ellipsoid(m,tuple(t*.86 for t in v),(.105,.105,.105),'esfera_oscura',9,5)
 torus(m,(0,0,0),.96,.025,'esfera_clara',24,4);return m

def marker():
 m=mesh();torus(m,(0,.03,0),1.17,.05,'COLOR_DINAMICO',30,5);return m

def desert_ground():
 m=mesh();box(m,(0,-24,-65),(110,.5,180),'arenisca_oscura')
 for i in range(34):
  x=(i*37%94)-47;z=-(i*43%163)+18
  if abs(x)<17:continue
  cylinder(m,(x,-23.7,z),1.2,.6,'arena',7,r_top=.65)
 return m

def main():
 specs=[]
 for k,(a,b) in enumerate([(0,40),(40,74),(74,96),(96,130)]):
  specs.append((f'tramo_pista_{k+1}',track(a,b),'origen global',f'Curva S={a}..{b}; colocar en (0,0,0).'))
  for side,label in [(-1,'izquierda'),(1,'derecha')]:specs.append((f'pared_{label}_{k+1}',wall(a,b,side),'origen global',f'Lado {label} S={a}..{b}; colocar en (0,0,0).'))
 for i in range(3):specs.append((f'puente_roto_{i+1}',bridge(i),'origen global',f'Franja segura grieta {i+1}; colocar en (0,0,0).'))
 specs += [('rampa_atajo',ramp(),'origen global','S=90..93.5 lateral 2..3.6.'),('arco_natural',rock_arch(),'origen global','S=12; colocar en (0,0,0).'),('mesa_lejana',mesa(),'pie central','Decorar X=±20..30.'),('cactus',cactus(),'pie central','Obstáculo y decorado; radio lógico .5.'),('roca_caida',fallen_rock(),'pie central','Obstáculo; radio lógico .9.'),('banderin_checkpoint',checkpoint(),'pie central','Instanciar lateral ±3.9 en S=28,58,88.'),('arco_salida',gate(False),'pie central','S=.8, orientar eje X perpendicular a tangente.'),('arco_meta',gate(True),'pie central','S=120, orientar eje X perpendicular a tangente.'),('esfera_piedra',sphere(),'centro','Centro Y=elevación+1; rotar sobre eje de rodadura.'),('aro_jugador',marker(),'centro','Teñir con color de jugador.'),('suelo_desertico',desert_ground(),'origen global','Decorado sin colisión; colocar en (0,0,0).')]
 for name,m,pivot,usage in specs:save(name,m,pivot,usage)
 (ROOT/'manifest.json').write_text(json.dumps({'minijuego':'Esferas del Cañón','base':'claude/expansion-party ea5472c','longitud_curva':round(S[-1],4),'meta_s':120,'modelos':MANIFEST},ensure_ascii=False,indent=2))
 print(len(specs),'GLB;',sum(a['triangulos'] for a in MANIFEST),'triángulos; longitud',round(S[-1],2))
if __name__=='__main__':main()
