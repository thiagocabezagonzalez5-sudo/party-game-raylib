"""Arte original procedural para Banquete Turbo. Python 3, solo biblioteca estandar.
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
    doc={'asset':{'version':'2.0','generator':'Banquete Turbo - generador original v1'},
         'scene':0,'scenes':[{'nodes':[0]}],'nodes':[{'name':name,'mesh':0}],
         'meshes':[{'name':name,'primitives':primitives}],'materials':materials,
         'buffers':[{'byteLength':len(blob)}],'bufferViews':views,'accessors':accessors}
    text=json.dumps(doc,separators=(',',':')).encode();text+=b' '*((-len(text))%4)
    data=struct.pack('<III',0x46546c67,2,12+8+len(text)+8+len(blob))+struct.pack('<II',len(text),0x4e4f534a)+text+struct.pack('<II',len(blob),0x004e4942)+blob
    (OUT/(name+'.glb')).write_bytes(data)
    lo=[round(min(v[j] for v in bounds),4) for j in range(3)];hi=[round(max(v[j] for v in bounds),4) for j in range(3)]
    MANIFEST.append({'archivo':name+'.glb','triangulos':sum(len(v) for v in m.values()),'materiales':list(m),
      'bytes':len(data),'min':lo,'max':hi,'pivote':pivot,'uso':usage})
