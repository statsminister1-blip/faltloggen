# SPDX-License-Identifier: CERN-OHL-S-2.0
# Skal i ett stycke med snabbfäste för mobilhållaren i Fendt-hytten (Richter/HR "4QuickFix"), v3
#
# v3: hålbild efter uppmätt fäste: hålen 18 mm höga, slits 3 mm, utbuktning 6 mm lång och 3 mm ut (hålet 6 mm
#   brett), 48 mm ytter-ytter på höjden och 24 mm ytter-ytter på bredden. Fästet sitter till VÄNSTER sett framifrån.
#   OBS: i skriptets koordinater pekar +x åt VÄNSTER sett framifrån (z ökar bakåt) – se nedan.
#   Skärmen passar i skalet (provat). Hålbilden i v3 är INTE provad mot fästet än.
# v2: fästplattan är inbyggd i bakväggen – skalet är EN del, inga skruvar till fästet.
#   Hålen går rakt genom bakväggen; runt varje hål är väggen 2,5 mm tjock (fickor inifrån) så att klorna
#   griper som i mobilhållaren.
# för Waveshare ESP32-S3-Touch-LCD-7
#
# - Ingen främre ram: glaset fyller hela framsidan. Väggen går runt glasets kant och slutar
#   jäms med glasets framsida (skyddar kanterna, täcker inget av bilden).
# - Skärmen läggs i framifrån och vilar på fyra stöd i hörnen (glasets kant utanför bakplåten).
#   4 x M3x8 (cylinderhuvud) bakifrån i skärmens egna M3-insatser drar glaset mot stöden.
# - Kablarna (ISOBUS 2-ledare och USB) går in genom hål i nederkanten och kläms INNE i skalet
#   mot bakväggen med var sin klämma (2 x M3 vardera) = dragavlastning. USB-hålet är avlångt så
#   att USB-C-kontakten går att trä in utifrån.
# - Baksidan: samma fyra hål som mobilhållaren (Richter/HR "4QuickFix") direkt i bakväggen. Bakom hålen
#   finns fickor där fästets klor får plats (väggen 2,5 mm vid hålen).
#   Låset sitter i traktorns fasta fäste och släpps med de två vingarna på dess sidor,
#   så plattan behöver bara hålen – inga rörliga delar i skalet.
#   Hålen sitter förskjutna åt vänster och nedåt (sett framifrån) så att skalet går fritt
#   från armstödet.
# - Spåren för ett DC/DC-kort (Electrokit EKM006) längs väggen finns kvar som i bygg_skal_robust.py.
#
# Koordinater som i bygg_skal_robust.py: y uppåt, origo = glasets mitt, z = 0 vid glasets framsida, z ökar BAKÅT.
# Därmed pekar +x åt HÖGER sett BAKIFRÅN = åt VÄNSTER sett FRAMIFRÅN.
#
# Utskrift: skalet med baksidan nedåt på plattan (inga stöd behövs). Klämmorna platt.
import trimesh, numpy as np
from shapely.geometry import box as sbox, Point
from shapely.ops import unary_union
import os
OUT=os.environ.get('SKAL_OUT','out')
os.makedirs(OUT, exist_ok=True)
U=lambda m: trimesh.boolean.union(m,engine='manifold')
Dif=lambda m: trimesh.boolean.difference(m,engine='manifold')
I=lambda m: trimesh.boolean.intersection(m,engine='manifold')

def rrect(w,h,r,cx=0,cy=0):
    return sbox(cx-w/2+r, cy-h/2+r, cx+w/2-r, cy+h/2-r).buffer(r, resolution=24)
def ext(poly,z0,z1):
    gs = list(poly.geoms) if hasattr(poly,'geoms') else [poly]
    ms=[]
    for g in gs:
        if g.area < 1e-3: continue
        m=trimesh.creation.extrude_polygon(g, z1-z0); m.apply_translation([0,0,z0]); ms.append(m)
    return ms[0] if len(ms)==1 else U(ms)
def hull_rr(w,h,r,z,w2,h2,r2,z2):
    a=np.array(rrect(w,h,r).exterior.coords)[:-1]; b=np.array(rrect(w2,h2,r2).exterior.coords)[:-1]
    return trimesh.convex.convex_hull(np.vstack([np.c_[a,np.full(len(a),z)], np.c_[b,np.full(len(b),z2)]]))
def cylz(r,z0,z1,x=0,y=0,sec=48):
    c=trimesh.creation.cylinder(radius=r,height=z1-z0,sections=sec); c.apply_translation([x,y,(z0+z1)/2]); return c
def stadium(cx,cy,half,r):
    return sbox(cx-half,cy-0.001,cx+half,cy+0.001).buffer(r,resolution=24)
def through_bottom(poly_xz, y0, length):
    """2D-form i (x,z) som hål genom nederkanten, från y0 och 'length' mm nedåt (-y)."""
    m=trimesh.creation.extrude_polygon(poly_xz, length)
    M=np.eye(4); M[:3,0]=[1,0,0]; M[:3,1]=[0,0,1]; M[:3,2]=[0,-1,0]; M[:3,3]=[0,y0,0]
    m.apply_transform(M); return m

# ---------------- skärmen (samma mått som bygg_skal_robust.py)
GW, GH   = 192.96, 110.76
GT       = 2.0                        # glasets tjocklek, glaset ligger z = 0 .. 2
CL       = 0.3
PW, PH   = GW+2*CL, GH+2*CL          # glasficka 193,56 x 111,36
R_POCK   = 6.0
WALL     = 3.0                        # väggen runt glaset
OW, OH   = PW+2*WALL, PH+2*WALL      # 199,56 x 117,36
R_OUT    = R_POCK+WALL
XI, YI   = PW/2, PH/2                 # väggarnas insida: x ±96,78, y ±55,68
Z_INS    = 17.0                       # M3-insatsernas topp bakom glasets framsida
Z_BACK   = 24.0                       # bakväggens insida
T_BACK   = 4.0
Z_END    = Z_BACK+T_BACK              # baksidan 28 mm bakom glasets framsida
BACK_CH  = 1.5                        # fas runt baksidan
FRONT_CH = 0.6                        # liten fas på väggens framkant
BP_X     = 165.72/2 + 1.5             # bakplåtens halva bredd + 1,5 mm osäkerhet = 84,36
INS_X = [-63.1+0.4, 63.1+0.4]; INS_Y = [-32.40, 33.25]; INS_SLOT = 1.6
Z_SEAT   = 22.0                       # skruvhuvudets anliggning (M3x8 når 3 mm in i insatsen)

# ---------------- kablar
USB_X, USB_W, USB_H = 75.0, 13.5, 8.0  # avlångt hål: USB-C-kontakten träs in utifrån
ISO_X, ISO_D        = -70.0, 7.6       # ISOBUS 2-ledare, avskalade trådar till skruvplinten
Z_CAB    = 19.0                        # kabelhålens mitt i djupled (bakom skärmens bakplåt)
CLAMP_Y  = -YI+9.5
CLAMP = [(USB_X, 7.0, 2.5), (ISO_X, 8.5, 3.5)]  # (x, halvt skruvavstånd, höjd) – kablar ca 3,4 resp. 4,9 mm

# ---------------- DC/DC (EKM006) som i bygg_skal_robust.py
DC_Y0 = 5.0; DC_X0 = -XI+2.0

# ---------------- SNABBFÄSTE (v3)
MX, MY   = +36.0, -15.0               # fästets mitt: +x = 36 mm åt VÄNSTER sett framifrån, 15 mm nedåt
HOLE_CL  = 0.15                       # spel runt hålet (utskriften krymper hål något)
SLIT_W   = 3.0                        # smal slits (längs höjden)
SLIT_L   = 18.0                       # hålets höjd
BULGE_W  = 6.0                        # slits + utbuktning tillsammans (utbuktningen 3 mm ut)
BULGE_L  = 6.0                        # utbuktningens längd, mitt på slitsen
HOOK_DX  = 24.0 - BULGE_W             # 24 mm ytter-ytter i sidled -> hålens mitt-avstånd 18
HOOK_DY  = 48.0 - SLIT_L              # 48 mm ytter-ytter i höjdled -> hålens mitt-avstånd 30
PL_T     = 2.5                        # väggens tjocklek vid hålen (= godset klorna griper om)
POCK_W, POCK_H = 12.0, 22.0           # ficka bakom varje hål för klorna
HOOKS = [(MX+sx*HOOK_DX/2, MY+sy*HOOK_DY/2, sx) for sx in (-1,1) for sy in (-1,1)]

def hole_shape(cx, cy, sx):
    """Ett hål: utbuktningen inåt (mot mitten), slitsen på utsidan. Hela hålets bredd centrerad på klon."""
    x_out = cx + sx*BULGE_W/2                 # hålets yttre kant
    slit  = sbox(min(x_out, x_out-sx*SLIT_W), cy-SLIT_L/2, max(x_out, x_out-sx*SLIT_W), cy+SLIT_L/2)
    bulge = sbox(cx-BULGE_W/2, cy-BULGE_L/2, cx+BULGE_W/2, cy+BULGE_L/2)
    return unary_union([slit, bulge]).buffer(HOLE_CL, join_style=2)

# ================= SKAL
outer = U([hull_rr(OW-2*FRONT_CH,OH-2*FRONT_CH,R_OUT-FRONT_CH,0.0, OW,OH,R_OUT,FRONT_CH),
           ext(rrect(OW,OH,R_OUT), FRONT_CH-0.01, Z_END-BACK_CH),
           hull_rr(OW,OH,R_OUT,Z_END-BACK_CH-0.01, OW-2*BACK_CH,OH-2*BACK_CH,R_OUT-BACK_CH,Z_END)])
cav = ext(rrect(PW,PH,R_POCK), -1, Z_BACK)
parts=[Dif([outer, cav])]
pocket2d = rrect(PW+0.02,PH+0.02,R_POCK)
# stöd under glasets kant i de fyra hörnen (utanför bakplåten, fritt från DC/DC-kortet på vänster sida)
for sx in (-1,1):
    for sy in (-1,1):
        pad = sbox(min(sx*(BP_X+0.8), sx*(XI+1)), min(sy*37.5, sy*(YI+1)),
                   max(sx*(BP_X+0.8), sx*(XI+1)), max(sy*37.5, sy*(YI+1))).intersection(pocket2d)
        parts.append(ext(pad, GT, Z_BACK+0.01))
# fästen mot skärmens M3-insatser
for x in INS_X:
    for y in INS_Y: parts.append(ext(stadium(x,y,INS_SLOT,5.0), Z_INS+0.1, Z_BACK+0.01))
# spår för DC/DC-kortet
for sgn in (-1,1):
    ye = DC_Y0 + sgn*30.0
    blk = ext(sbox(-XI-0.01, min(ye-sgn*1.5, ye+sgn*3.5), -XI+6.0, max(ye-sgn*1.5, ye+sgn*3.5)), 3.0, Z_BACK+0.01)
    slot= ext(sbox(DC_X0-0.2, min(ye-sgn*1.6, ye+sgn*0.4), DC_X0+1.6+0.2, max(ye-sgn*1.6, ye+sgn*0.4)), 2.0, Z_BACK-0.5)
    parts.append(Dif([blk, slot]))
# fästen för kabelklämmorna (inne i skalet, på bakväggen)
for (x,hs,hh) in CLAMP:
    for s in (-1,1): parts.append(cylz(3.2, Z_BACK-hh, Z_BACK+0.01, x+s*hs, CLAMP_Y))
shell = U(parts)

cut=[]
# skruvhål till skärmens insatser (avlånga i x), huvudet försänkt till Z_SEAT
for x in INS_X:
    for y in INS_Y:
        cut += [ext(stadium(x,y,INS_SLOT,1.75), Z_INS-1, Z_END+1), ext(stadium(x,y,INS_SLOT,3.3), Z_SEAT, Z_END+1)]
# kabelhål i nederkanten
usb2d = rrect(USB_W, USB_H, USB_H/2-0.01, USB_X, Z_CAB)
iso2d = Point(ISO_X, Z_CAB).buffer(ISO_D/2, resolution=32)
for p in (usb2d, iso2d): cut.append(through_bottom(p, -YI+1.0, WALL+3))
# kabelklämmornas skruvhål (gängas av M3)
for (x,hs,hh) in CLAMP:
    for s in (-1,1): cut.append(cylz(1.25, Z_BACK-hh-1, Z_BACK+2.0, x+s*hs, CLAMP_Y))
# snabbfästets hål rakt genom bakväggen + fickor inifrån för klorna (väggen PL_T vid hålen)
for (cx,cy,sx) in HOOKS:
    cut.append(ext(rrect(POCK_W, POCK_H, 2.0, cx, cy), Z_BACK-1, Z_END-PL_T))
    cut.append(ext(hole_shape(cx,cy,sx), Z_BACK-1, Z_END+1))
shell = Dif([shell]+cut)

# ================= KABELKLÄMMOR (som i bygg_skal_robust.py)
def clamp(hs, d_max, name):
    L=2*hs+9; bar=ext(rrect(L,8,2.5),0,4.0)
    v=trimesh.convex.convex_hull(np.array([[-d_max*0.6,-5,-0.01],[d_max*0.6,-5,-0.01],[0,-5,d_max*0.2],
                                           [-d_max*0.6,5,-0.01],[d_max*0.6,5,-0.01],[0,5,d_max*0.2]]))
    hl=[cylz(1.7,-1,5,s*hs,0) for s in (-1,1)]
    m=Dif([bar,v]+hl); m.export(OUT+f'/{name}.stl'); return m
clamp(7.0, 5.0, 'skal7s_klamma_usb'); clamp(8.5, 7.5, 'skal7s_klamma_isobus')

shell.export(OUT+'/skal7s_skal.stl')

# ================= KONTROLLER
print('skal     ', shell.is_watertight, shell.bounds.round(2).tolist(), round(shell.volume/1000,1),'cm3')
glass = ext(rrect(GW,GH,7.0), 0, GT)
bp_y1 = GH/2-3.8
module = ext(sbox(-165.72/2-1.5, bp_y1-97.6, 165.72/2+1.5, bp_y1), GT, 12.5)
ins = [cylz(2.5, 12.5, Z_INS, x, y) for x in (-63.1,63.1) for y in INS_Y]
pcb = ext(sbox(-49.1-6, -43.1, 56.9+1, 25.0+3), 12.5, Z_INS-0.2)
dcdc=trimesh.creation.box(extents=[1.6,59.4,19.6]); dcdc.apply_translation([DC_X0+0.8, DC_Y0, Z_BACK-0.5-9.8])
dcdc_k=trimesh.creation.box(extents=[8.0,52,18]); dcdc_k.apply_translation([DC_X0+1.6+4.0, DC_Y0, Z_BACK-0.5-9.8])
bars=[]
for (x,hs,hh) in CLAMP:   # klämmorna på plats
    b=ext(rrect(2*hs+9,8,2.5,x,CLAMP_Y), Z_BACK-hh-4.0, Z_BACK-hh); bars.append(b)
nkr=0
for nm,b in [('glas',glass),('bakplåt',module),('kort',pcb),('DC/DC',dcdc),('DC/DC-komp',dcdc_k)]+\
            [('insats',i) for i in ins]+[('klämma',b) for b in bars]:
    for pn,p in [('skal',shell)]:
        x=I([b,p]); v=x.volume if len(x.faces) else 0
        if v>0.01 and not (nm=='klämma' and pn=='skal'): print('KROCK', nm, pn, round(v,2)); nkr+=1
for i,a in enumerate(bars):
    for nm,b in [('kort',pcb),('bakplåt',module)]+[('insats',k) for k in ins]:
        x=I([a,b])
        if len(x.faces) and x.volume>0.01: print('KROCK klämma', nm); nkr+=1
print('krockar:', nkr)
# skärmen läggs i framifrån
mod_all = U([glass, module, pcb] + ins)
stuck=[]
for dz in np.arange(-40, 0.01, 0.5):
    m=mod_all.copy(); m.apply_translation([0,0,dz]); x=I([shell,m])
    if len(x.faces) and x.volume>0.01: stuck.append(round(float(dz),1))
print('montering skärm framifrån:', 'OK' if not stuck else f'FASTNAR vid {stuck[:5]}')
print('glaset vilar på stöden vid z =', GT, '; insatsens topp', Z_INS, 'mot fästets yta', Z_INS+0.1)
# väggens framkant jäms med glaset
print('skalets framkant z min', round(shell.bounds[0][2],2), '(glasets framsida 0)')
# hur långt skalet når från fästets mitt
print(f'från fästets mitt (sett framifrån): vänster {OW/2-MX:.1f} mm, höger {OW/2+MX:.1f}, nedåt {OH/2+MY:.1f}, uppåt {OH/2-MY:.1f}')
# godset runt klorna: tjocklek vid varje hålkant
for (cx,cy,sx) in HOOKS:
    ray=trimesh.ray.ray_triangle.RayMeshIntersector(shell)
    pts=[]
    for (dx,dy) in [(0,-SLIT_L/2-1.0),(0,SLIT_L/2+1.0),(-sx*(BULGE_W/2+1.0),0)]:
        loc,_,_=ray.intersects_location([[cx+dx,cy+dy,Z_END+5]],[[0,0,-1]])
        zs=sorted(loc[:,2]); pts.append(round(Z_END-zs[-2],2) if len(zs)>1 else None)
    print('gods vid hål', (round(cx,1),round(cy,1)), pts)
print('kontroll klar')
