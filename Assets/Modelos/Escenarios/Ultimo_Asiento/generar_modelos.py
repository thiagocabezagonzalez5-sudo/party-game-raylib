"""Arte original procedural para Ultimo Asiento. Python 3, solo biblioteca estandar.
GLB 2.0: Y arriba, metros/unidades raylib, normales planas, indices uint16.
Ejecutar: python generar_modelos.py. No modifica el juego.
"""
from pathlib import Path
from collections import defaultdict
import math, struct, json

ROOT = Path(__file__).resolve().parent
OUT = ROOT / 'GLB'
OUT.mkdir(parents=True, exist_ok=True)
TAU = math.tau
PALETTE = {
 'cereza': '#ca3e67', 'cereza_oscura': '#792e52', 'azul': '#4387cb',
 'azul_oscuro': '#254971', 'turquesa': '#4abab8', 'crema': '#fff0d2',
 'oro': '#e5b550', 'oro_oscuro': '#ae7639', 'madera': '#ae704b',
 'madera_clara': '#d39966', 'madera_oscura': '#61443d', 'metal': '#b6c8d6',
 'metal_oscuro': '#3d4b65', 'blanco': '#fff8e9', 'negro': '#273344',
 'violeta': '#7755a1', 'violeta_oscuro': '#503b79', 'rosa': '#ea91b5',
 'COLOR_DINAMICO': '#ffffff', 'BOMBILLAS': '#ffffff',
}
def add(a,b): return tuple(x+y for x,y in zip(a,b))
def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def mul(a,s): return tuple(x*s for x in a)
def dot(a,b): return sum(x*y for x,y in zip(a,b))
def cross(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def unit(a):
    n=math.sqrt(dot(a,a)); return mul(a,1/n) if n>1e-10 else (0,1,0)
def mesh(): return defaultdict(list)
def tri(m,a,b,c,mat):
    n=cross(sub(b,a),sub(c,a))
    if dot(n,n)>1e-16: m[mat].append((a,b,c))
def quad(m,a,b,c,d,mat): tri(m,a,b,c,mat);tri(m,a,c,d,mat)
def box(m,p,size,mat):
    x,y,z=p; w,h,d=[s/2 for s in size]
    v=[(x-w,y-h,z-d),(x+w,y-h,z-d),(x+w,y+h,z-d),(x-w,y+h,z-d),
       (x-w,y-h,z+d),(x+w,y-h,z+d),(x+w,y+h,z+d),(x-w,y+h,z+d)]
    for a,b,c,d in [(0,3,2,1),(4,5,6,7),(0,4,7,3),(1,2,6,5),(0,1,5,4),(3,7,6,2)]:
        quad(m,v[a],v[b],v[c],v[d],mat)
def cylinder(m,p,r,h,mat,n=24,r_top=None,caps=True):
    rt=r if r_top is None else r_top
    for i in range(n):
        a=TAU*i/n;b=TAU*(i+1)/n
        v0=add(p,(r*math.cos(a),0,r*math.sin(a)))
        v1=add(p,(r*math.cos(b),0,r*math.sin(b)))
        v2=add(p,(rt*math.cos(b),h,rt*math.sin(b)))
        v3=add(p,(rt*math.cos(a),h,rt*math.sin(a)))
        quad(m,v0,v3,v2,v1,mat)
        if caps:
            tri(m,p,v0,v1,mat);tri(m,add(p,(0,h,0)),v2,v3,mat)
def beam(m,a,b,r,mat,n=8,r_end=None):
    axis=unit(sub(b,a));u=unit(cross(axis,(0,0,1) if abs(axis[2])<.9 else (0,1,0)));v=cross(axis,u)
    rt=r if r_end is None else r_end
    for i in range(n):
        d0=add(mul(u,math.cos(i*TAU/n)),mul(v,math.sin(i*TAU/n)))
        d1=add(mul(u,math.cos((i+1)*TAU/n)),mul(v,math.sin((i+1)*TAU/n)))
        p0=add(a,mul(d0,r));p1=add(a,mul(d1,r));p2=add(b,mul(d1,rt));p3=add(b,mul(d0,rt))
        quad(m,p0,p1,p2,p3,mat);tri(m,a,p1,p0,mat);tri(m,b,p3,p2,mat)
def ellipsoid(m,p,size,mat,n=12,rings=6):
    def pt(i,j):
        a=i*TAU/n;t=math.pi*j/rings
        return add(p,(size[0]*math.sin(t)*math.cos(a),size[1]*math.cos(t),size[2]*math.sin(t)*math.sin(a)))
    for j in range(rings):
        for i in range(n): quad(m,pt(i,j),pt(i+1,j),pt(i+1,j+1),pt(i,j+1),mat)
def torus(m,p,r,tube,mat,n=32,k=6,plane='xz',start=0,end=TAU):
    def pt(i,j):
        a=start+(end-start)*i/n;b=TAU*j/k
        x=(r+tube*math.cos(b))*math.cos(a);y=tube*math.sin(b);z=(r+tube*math.cos(b))*math.sin(a)
        if plane=='xy': return add(p,(x,z,-y))
        if plane=='yz': return add(p,(-y,x,z))
        return add(p,(x,y,z))
    for i in range(n):
        for j in range(k): quad(m,pt(i,j),pt(i,j+1),pt(i+1,j+1),pt(i+1,j),mat)
def ring(m,y,r0,r1,mat,n=48):
    for i in range(n):
        a=i*TAU/n;b=(i+1)*TAU/n
        quad(m,(r0*math.cos(a),y,r0*math.sin(a)),(r0*math.cos(b),y,r0*math.sin(b)),
             (r1*math.cos(b),y,r1*math.sin(b)),(r1*math.cos(a),y,r1*math.sin(a)),mat)
def transform(m,p=(0,0,0),angle=0):
    c=math.cos(angle);s=math.sin(angle);out=mesh()
    for mat,ts in m.items():
        for t in ts: out[mat].append(tuple(add((v[0]*c+v[2]*s,v[1],-v[0]*s+v[2]*c),p) for v in t))
    return out
def merge(m,other):
    for k,v in other.items():m[k].extend(v)
def star(m,p,r,mat,plane='xy',thick=.035):
    points=[]
    for i in range(10):
        a=math.pi/2+i*TAU/10; rr=r if i%2==0 else r*.44
        points.append((rr*math.cos(a),rr*math.sin(a)))
    # Estrella solida, orientada al frente +Z, con reverso y cantos.
    for i in range(10):
        a=points[i];b=points[(i+1)%10]
        v0=add(p,(a[0],a[1],thick/2));v1=add(p,(b[0],b[1],thick/2))
        v2=add(p,(b[0],b[1],-thick/2));v3=add(p,(a[0],a[1],-thick/2))
        tri(m,add(p,(0,0,thick/2)),v0,v1,mat)
        tri(m,add(p,(0,0,-thick/2)),v2,v3,mat);quad(m,v0,v3,v2,v1,mat)

MANIFEST=[]
def save(name,m,pivot,usage):
    blob=bytearray();views=[];accessors=[];primitives=[];materials=[];bounds=[]
    def acc(values,typ,ctype,fmt,components,target=None,limits=False):
        while len(blob)%4:blob.append(0)
        start=len(blob);flat=[a for v in values for a in (v if isinstance(v,tuple) else (v,))]
        blob.extend(struct.pack('<'+fmt*len(flat),*flat))
        view={'buffer':0,'byteOffset':start,'byteLength':len(blob)-start}
        if target:view['target']=target
        views.append(view);a={'bufferView':len(views)-1,'componentType':ctype,'count':len(values),'type':typ}
        if limits:a.update(min=[min(v[j] for v in values) for j in range(components)],max=[max(v[j] for v in values) for j in range(components)])
        accessors.append(a);return len(accessors)-1
    light=unit((-.45,.85,.5))
    for mat,ts in m.items():
        rgb=tuple(int(PALETTE[mat][i:i+2],16)/255 for i in (1,3,5))
        material={'name':mat,'pbrMetallicRoughness':{'baseColorFactor':[*rgb,1],'metallicFactor':0,'roughnessFactor':.85}}
        materials.append(material)
        # Un primitive nunca supera el limite uint16 de raylib.
        for offset in range(0,len(ts),20000):
            pts=[];norm=[];colors=[]
            for t in ts[offset:offset+20000]:
                n=unit(cross(sub(t[1],t[0]),sub(t[2],t[0])))
                shade=1 if mat=='BOMBILLAS' else .76+.24*max(0,dot(n,light))
                pts.extend(t);norm.extend([n]*3);colors.extend([(shade,shade,shade,1.)]*3)
            bounds.extend(pts)
            primitives.append({'attributes':{'POSITION':acc(pts,'VEC3',5126,'f',3,34962,True),
              'NORMAL':acc(norm,'VEC3',5126,'f',3,34962),'COLOR_0':acc(colors,'VEC4',5126,'f',4,34962)},
              'indices':acc(list(range(len(pts))),'SCALAR',5123,'H',1,34963),'material':len(materials)-1,'mode':4})
    while len(blob)%4:blob.append(0)
    doc={'asset':{'version':'2.0','generator':'Ultimo Asiento - generador original v1'},
         'scene':0,'scenes':[{'nodes':[0]}],'nodes':[{'name':name,'mesh':0}],
         'meshes':[{'name':name,'primitives':primitives}],'materials':materials,
         'buffers':[{'byteLength':len(blob)}],'bufferViews':views,'accessors':accessors}
    text=json.dumps(doc,separators=(',',':')).encode();text+=b' '*((-len(text))%4)
    data=struct.pack('<III',0x46546c67,2,12+8+len(text)+8+len(blob))+struct.pack('<II',len(text),0x4e4f534a)+text+struct.pack('<II',len(blob),0x004e4942)+blob
    (OUT/(name+'.glb')).write_bytes(data)
    lo=[round(min(v[j] for v in bounds),4) for j in range(3)];hi=[round(max(v[j] for v in bounds),4) for j in range(3)]
    MANIFEST.append({'archivo':name+'.glb','triangulos':sum(len(v) for v in m.values()),'materiales':list(m),
      'bytes':len(data),'min':lo,'max':hi,'pivote':pivot,'uso':usage})

def carousel_base():
    m=mesh();cylinder(m,(0,0,0),2.1,.43,'cereza',48)
    cylinder(m,(0,.02,0),2.16,.08,'oro_oscuro',48);cylinder(m,(0,.40,0),2.2,.10,'oro',48)
    cylinder(m,(0,.50,0),2.12,.03,'madera_clara',48,caps=False)
    for r0,r1,mat in [(0,.49,'madera_clara'),(.49,.55,'oro'),(.55,1.97,'madera_clara'),(1.97,2.1,'crema'),(2.1,2.12,'madera_clara')]:
        ring(m,.53,r0,r1,mat)
    for i in range(24):
        a=TAU*i/24;piece=mesh();star(piece,(0,.25,2.108),.105,'crema',thick=.018)
        merge(m,transform(piece,angle=a))
    return m

def column():
    m=mesh();cylinder(m,(0,.53,0),.39,.16,'oro',12)
    cylinder(m,(0,.69,0),.32,2.4,'crema',12);cylinder(m,(0,3.03,0),.42,.17,'oro',12)
    for i in range(6):
        a=i*TAU/6;panel=mesh();box(panel,(0,1.85,.305),(.24,1.5,.055),'turquesa');
        box(panel,(0,1.03,.33),(.27,.065,.06),'oro');box(panel,(0,2.67,.33),(.27,.065,.06),'oro')
        merge(m,transform(panel,angle=a))
    return m

def roof():
    m=mesh();n=48
    cylinder(m,(0,3.14,0),2.40,.16,'azul_oscuro',48)
    for i in range(n):
        a=TAU*i/n;b=TAU*(i+1)/n;mat=['azul','crema','cereza','crema'][(i//3)%4]
        # Perfil convexo en dos anillos: tela con volumen, cara exterior arriba.
        rings=[(2.43,3.30),(1.65,3.60),(.18,4.20)]
        for (r0,y0),(r1,y1) in zip(rings,rings[1:]):
            quad(m,(r0*math.cos(a),y0,r0*math.sin(a)),(r1*math.cos(a),y1,r1*math.sin(a)),
                 (r1*math.cos(b),y1,r1*math.sin(b)),(r0*math.cos(b),y0,r0*math.sin(b)),mat)
        tri(m,(0,3.30,0),(2.43*math.cos(a),3.30,2.43*math.sin(a)),(2.43*math.cos(b),3.30,2.43*math.sin(b)),'crema')
    torus(m,(0,3.29,0),2.42,.035,'oro',48)
    for i in range(24):
        a=TAU*i/24;ellipsoid(m,(2.4*math.cos(a),3.15,2.4*math.sin(a)),(.08,.12,.08),'oro',8,4)
    cylinder(m,(0,4.19,0),.19,.14,'oro',12,r_top=.09);ellipsoid(m,(0,4.4,0),(.18,.18,.18),'oro',12,6)
    return m

def horse():
    m=mesh();ellipsoid(m,(0,0,0),(.39,.21,.17),'blanco',12,6)
    beam(m,(.23,.06,0),(.38,.43,0),.135,'blanco',8,r_end=.1)
    ellipsoid(m,(.41,.43,0),(.18,.12,.10),'blanco',10,6)
    ellipsoid(m,(.54,.385,0),(.13,.07,.09),'crema',10,6)
    for z in [-.075,.075]:
        beam(m,(.33,.50,z),(.32,.65,z),.043,'blanco',6,r_end=.009)
        ellipsoid(m,(.445,.457,z*1.28),(.025,.028,.012),'negro',8,4)
    for x in [-.23,.22]:
        for z in [-.115,.115]:
            knee=(x+(.10 if x>0 else -.05),-.20,z)
            foot=(x+(.17 if x>0 else -.13),-.37,z)
            beam(m,(x,-.07,z),knee,.055,'blanco',7,r_end=.04)
            beam(m,knee,foot,.04,'blanco',7);box(m,add(foot,(.01,-.012,0)),(.13,.08,.09),'oro_oscuro')
    for i in range(5):
        ellipsoid(m,(.25+i*.015,.20+i*.067,0),(.075,.075,.13),'cereza',8,4)
    beam(m,(-.31,.07,0),(-.46,-.06,0),.07,'cereza',7,r_end=.055)
    beam(m,(-.46,-.06,0),(-.47,-.23,.015),.055,'cereza',7,r_end=.02)
    box(m,(-.025,.175,0),(.28,.05,.37),'cereza');box(m,(-.025,.21,0),(.22,.055,.27),'azul')
    for z in [-.182,.182]:
        beam(m,(.46,.39,z*.6),(.08,.14,z),.012,'oro',5)
        torus(m,(-.02,-.08,z),.065,.012,'oro',12,5,plane='xy')
    return m

def cup():
    m=mesh();cylinder(m,(0,0,0),1.2,.055,'oro',40);cylinder(m,(0,.055,0),1.12,.025,'crema',40)
    cylinder(m,(0,.08,0),.75,.05,'COLOR_DINAMICO',32)
    n=32
    # Seccion cerrada de porcelana; interior abierto y pared baja legible.
    profile=[(.75,.13),(.91,.35),(.98,.70),(.91,.70),(.84,.36),(.69,.17)]
    for j in range(len(profile)-1):
        r0,y0=profile[j];r1,y1=profile[j+1]
        for i in range(n):
            a=i*TAU/n;b=(i+1)*TAU/n
            quad(m,(r0*math.cos(a),y0,r0*math.sin(a)),(r1*math.cos(a),y1,r1*math.sin(a)),
                 (r1*math.cos(b),y1,r1*math.sin(b)),(r0*math.cos(b),y0,r0*math.sin(b)),
                 'crema' if j>=2 else 'COLOR_DINAMICO')
    cylinder(m,(0,.13,0),.70,.04,'crema',32)
    torus(m,(0,.70,0),.945,.037,'oro',32,6)
    torus(m,(1.005,.43,0),.175,.055,'COLOR_DINAMICO',18,6,plane='xy')
    for a in [0,TAU/3,2*TAU/3]:
        s=mesh();star(s,(0,.42,.925),.13,'oro',thick=.025);merge(m,transform(s,angle=a))
    return m

def fence():
    m=mesh()
    for x in [-.84,.84]:
        cylinder(m,(x,0,0),.09,.78,'crema',8);cylinder(m,(x,.06,0),.13,.07,'oro',8)
        cylinder(m,(x,.76,0),.12,.055,'oro',8);ellipsoid(m,(x,.90,0),(.13,.13,.13),'BOMBILLAS',10,5)
    for y in [.28,.57]:beam(m,(-.84,y,0),(.84,y,0),.045,'oro',6)
    for x in [-.42,0,.42]:
        beam(m,(x,.28,0),(x,.57,0),.028,'crema',6)
        ellipsoid(m,(x,.6,0),(.055,.055,.055),'cereza',8,4)
    return m

def ferris_support():
    m=mesh()
    for z in [-.65,.65]:
        for x in [-1.65,1.65]:
            box(m,(x,.10,z),(.58,.2,.68),'oro_oscuro')
            beam(m,(x,.2,z),(0,6.4,z),.13,'metal',8)
        beam(m,(-1.3,1.4,z),(1.3,1.4,z),.08,'turquesa',8)
        beam(m,(-.9,3,z),(.9,3,z),.065,'turquesa',8)
    beam(m,(0,6.4,-.85),(0,6.4,.85),.25,'oro',12)
    return m

def ferris_wheel():
    m=mesh()
    for z in [-.30,.30]:
        torus(m,(0,0,z),5,.085,'cereza',48,6,plane='xy')
        torus(m,(0,0,z),4.65,.045,'oro',48,5,plane='xy')
        beam(m,(0,0,z-.08),(0,0,z+.08),.38,'azul',16)
        for i in range(8):
            a=i*TAU/8;p=(5*math.cos(a),5*math.sin(a),z)
            beam(m,(0,0,z),p,.055,'crema',6)
            ellipsoid(m,p,(.13,.13,.13),'BOMBILLAS',8,4)
    for i in range(16):
        a=i*TAU/16;beam(m,(5*math.cos(a),5*math.sin(a),-.3),(5*math.cos(a),5*math.sin(a),.3),.065,'oro',6)
    return m

def gondola():
    m=mesh();beam(m,(0,0,-.22),(0,0,.22),.055,'oro',8)
    for x in [-.34,.34]:
        for z in [-.28,.28]: beam(m,(x,-.86,z),(x,-.13,z),.03,'metal',6)
    box(m,(0,-.86,0),(.8,.13,.7),'azul_oscuro')
    for z in [-.32,.32]:box(m,(0,-.69,z),(.8,.26,.08),'COLOR_DINAMICO')
    for x in [-.36,.36]:box(m,(x,-.68,0),(.08,.28,.6),'COLOR_DINAMICO')
    box(m,(0,-.60,0),(.64,.08,.36),'crema')
    box(m,(0,-.12,0),(.9,.10,.8),'COLOR_DINAMICO')
    beam(m,(0,-.03,0),(0,.04,0),.09,'oro',8)
    return m

def track():
    m=mesh()
    def p(t,z): return (-21+42*t,5+3.2*math.sin(t*9)+1.6*math.sin(t*23),z)
    for z in [-.35,.35]:
        for i in range(100):beam(m,p(i/100,z),p((i+1)/100,z),.08,'cereza',6)
    for i in range(81):
        t=i/80;beam(m,p(t,-.43),p(t,.43),.045,'oro',5)
    for i in range(21):
        t=i/20;px,py,_=p(t,0)
        for z in [-.46,.46]:
            beam(m,(px,0,z),(px,py-.06,z),.07,'metal',6)
        beam(m,(px,py-.1,-.60),(px,py-.1,.6),.07,'metal',6)
        if i<20:
            nx,ny,_=p((i+1)/20,0)
            for z in [-.46,.46]:beam(m,(px,.2,z),(nx,min(py,ny)*.72,z),.038,'metal_oscuro',5)
    return m

def coaster_car():
    m=mesh();box(m,(0,.17,0),(1.05,.22,.7),'oro');box(m,(0,.31,0),(.78,.12,.60),'cereza')
    box(m,(-.3,.52,0),(.13,.38,.60),'turquesa');box(m,(.44,.34,0),(.2,.25,.72),'oro')
    for z in [-.36,.36]:
        box(m,(0,.36,z),(1.0,.28,.08),'oro')
        for x in [-.3,.3]:beam(m,(x,.11,z-.055),(x,.11,z+.055),.11,'metal_oscuro',10)
    beam(m,(.15,.49,-.31),(.15,.49,.31),.035,'metal',6)
    return m

def stall():
    m=mesh();box(m,(0,.10,0),(3.4,.2,1.8),'madera_oscura')
    box(m,(0,.63,-.72),(3.3,1.05,.12),'madera');box(m,(0,.58,.72),(3.3,.95,.12),'cereza')
    for x in [-1.57,1.57]:box(m,(x,.63,0),(.12,1.05,1.50),'madera')
    for x in [-1.55,1.55]:
        for z in [-.72,.72]:box(m,(x,1.725,z),(.1,1.15,.1),'crema')
    box(m,(0,1.13,.75),(3.5,.14,.5),'madera_clara')
    for i in range(6):
        x0=-1.8+i*.6;x1=x0+.6;mat='cereza' if i%2==0 else 'crema'
        quad(m,(x0,2.15,1.1),(x1,2.15,1.1),(x1,2.62,0),(x0,2.62,0),mat)
        quad(m,(x0,2.62,0),(x1,2.62,0),(x1,2.15,-1.1),(x0,2.15,-1.1),mat)
        box(m,((x0+x1)/2,2.075,1.07),(.6,.15,.06),mat)
    for x in [-1.8,1.8]:
        tri(m,(x,2.15,-1.1),(x,2.62,0),(x,2.15,1.1),'cereza')
        tri(m,(x,2.15,1.1),(x,2.62,0),(x,2.15,-1.1),'cereza')
    box(m,(0,1.84,.79),(1.46,.35,.08),'azul_oscuro')
    for x in [-.45,0,.45]:star(m,(x,1.84,.85),.11,'oro')
    for i in range(7):
        x=-1.38+i*.46;mat='oro' if i%2 else 'crema'
        tri(m,(x-.18,.91,.80),(x+.18,.91,.80),(x,.64,.80),mat)
        tri(m,(x,.64,.80),(x+.18,.91,.80),(x-.18,.91,.80),mat)
    # Vasos y premios abstractos originales, sin marcas.
    for i in range(5):
        cylinder(m,(-.9+i*.43,1.22,.77),.09,.22,['turquesa','oro','crema'][i%3],8,r_top=.12)
    return m

def bleacher():
    m=mesh();box(m,(0,.32,0),(2.6,.64,5.6),'madera_oscura')
    for i in range(14):box(m,(0,.665,-2.6+i*.40),(2.6,.07,.382),'madera_clara' if i%2==0 else 'madera')
    for z in [-2.65,2.65]:
        for x in [-1.19,1.19]:box(m,(x,.93,z),(.08,.55,.08),'crema')
        beam(m,(-1.2,1.16,z),(1.2,1.16,z),.043,'oro',6)
    # Frente de acceso -X: escalones dentro de la huella 2.6 x 5.6.
    box(m,(-1.1,.15,0),(.4,.30,4.9),'madera_clara')
    for z in [-2,-1,0,1,2]:star(m,(0,.33,z),.12,'oro')
    return m

def balloon():
    m=mesh();ellipsoid(m,(0,0,0),(.32,.42,.32),'COLOR_DINAMICO',12,8)
    cylinder(m,(0,-.45,0),.06,.10,'COLOR_DINAMICO',6,r_top=.018)
    beam(m,(0,-.45,0),(.03,-1.1,0),.009,'crema',5)
    beam(m,(.03,-1.1,0),(-.02,-1.7,0),.009,'crema',5)
    return m

def arena():
    m=mesh();cylinder(m,(0,-.45,0),9,.43,'violeta_oscuro',96)
    cylinder(m,(0,-.02,0),8.5,.02,'violeta',96,caps=False)
    for r0,r1,mat in [(0,2.25,'violeta'),(2.25,2.29,'oro'),(2.29,4.42,'violeta'),(4.42,6.63,'violeta_oscuro'),(6.63,8.38,'violeta'),(8.38,8.48,'oro'),(8.48,8.5,'violeta')]:
        ring(m,0,r0,r1,mat,n=96)
    # Pocas marcas anchas: decoracion visible desde la camara alta.
    for i in range(24):
        a=i*TAU/24;piece=mesh();box(piece,(0,.018,8.10),(.07,.025,.20),'crema');merge(m,transform(piece,angle=a))
    return m

def main():
    specs=[
      ('carrusel_base',carousel_base(),'Centro a nivel del suelo','Escala 1; Y=-0.05. Radio maximo 2.2.'),
      ('carrusel_columna',column(),'Mismo origen que base','Escala 1; Y=-0.05. Pieza fija.'),
      ('carrusel_techo',roof(),'Mismo origen que base','Escala 1; Y=-0.05. Radio maximo 2.48, bajo el limite fisico 2.5.'),
      ('caballito',horse(),'Centro del cuerpo; frente +X','Colocar a radio 1.55 y altura 1.2; orientar tangencialmente. Vaiven vertical separado.'),
    ]
    pole=mesh();cylinder(pole,(0,.53,0),.045,2.62,'oro',8)
    bulb=mesh();ellipsoid(bulb,(0,0,0),(.13,.13,.13),'BOMBILLAS',12,6)
    specs += [('poste_caballito',pole,'Pie a nivel del carrusel','Colocar XZ a radio 1.55; Y=-0.05.'),
      ('bombilla',bulb,'Centro de la esfera','Reutilizar con DrawModel y tint para luces RGB; radio 0.13.'),
      ('taza',cup(),'Centro del platillo a nivel del suelo','Cambiar solo material COLOR_DINAMICO para ocupacion/trampa; pared baja sin colisiones nuevas.'),
      ('valla_tramo',fence(),'Centro del tramo, suelo','Ancho 1.94 con tapas; centros cada 1.70 aprox.; colocar tangente al radio 8.65.'),
      ('noria_soporte',ferris_support(),'Centro de la base, suelo','Trasladar a (15,0,-12.5). Centro del eje Y=6.4.'),
      ('noria_rueda',ferris_wheel(),'Centro del eje de giro','Colocar en (15,6.4,-12.5), girar sobre Z; radio 5.'),
      ('noria_cabina',gondola(),'Punto superior de suspension','8 instancias a radio 5. Mantener verticales mientras cambia su posicion.'),
      ('montana_rusa_vias',track(),'Centro XZ, suelo','Trasladar a Z=-17; recorrido X=-21..21, Y=5+3.2*sin(9t)+1.6*sin(23t).'),
      ('montana_rusa_carro',coaster_car(),'Centro de ruedas al nivel de los rieles','Frente +X. Colocar sobre el recorrido e inclinar alrededor de Z segun pendiente.'),
      ('puesto_feria',stall(),'Centro de la base, frente +Z','Colocar (-15+5.2*k,0,-11), k=0..2.'),
      ('grada',bleacher(),'Centro XZ, suelo','Colocar (+/-11.1,0,0.3). Plataforma superior Y=0.7; lado de acceso -X.'),
      ('globo',balloon(),'Centro del globo','Colores mediante COLOR_DINAMICO. Hilo incluido hacia -Y.'),
      ('arena',arena(),'Centro de la superficie de juego','Y=-0.05. Base decorativa opcional; no sustituye colisiones.')]
    for name,m,pivot,usage in specs:save(name,m,pivot,usage)
    (ROOT/'manifest.json').write_text(json.dumps({'version':1,'unidades':'1 unidad = 1 unidad del juego','ejes':'Y arriba, sistema derecho',
      'rama_referencia':'claude/expansion-party','commit_referencia':'ea5472c','licencia':'Arte original generado para este proyecto; sin recursos externos.',
      'modelos':MANIFEST},indent=2,ensure_ascii=False),encoding='utf-8')
    print(f'{len(MANIFEST)} GLB; {sum(x["triangulos"] for x in MANIFEST)} triangulos; {sum(x["bytes"] for x in MANIFEST)} bytes')

if __name__=='__main__':main()
