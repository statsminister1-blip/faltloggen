# SPDX-License-Identifier: CERN-OHL-S-2.0
# ISOBUS-plugg för hyttuttaget (TE/AMP CPC-uttag) – v2.2 med låsring, tre delar:
#  v2.2: hylsornas tunnlar bara 18 mm (bakom dem är kärnan ett ihåligt rör med gängan
#  utanpå), insatsens ytterdiameter +0,15 mm (mer friktion i uttaget). Fläns, krage, gänga och spåren för
#  låsringen är oförändrade -> låsring och kåpa från v2.1 passar.
#  KÄRNA:    en insats med stifthålen (k.stl, finns inte med här) (z 0–14, oförändrad passform) + fläns + krage + utvändig gänga
#  LÅSRING:  vrids ett halvt varv; två klackar följer uttagets två skruvspår och drar in kontakten
#  KÅPA:     skruvas på kärnan och stoppar mot kragen; håller låsringen på plats utan att klämma den
#
# Koordinater: z = 0 vid insatsens spets (längst in i uttaget), z ökar bakåt mot kabeln.
# Uttagets mått (uppmätta på en Fendt S4): hals Ø20,6, två spår mitt emot varandra, drygt 180°,
# spårdjup 1 mm, spårhöjd ca 2 mm, 6 mm från uttagets framkant till spårets bortre vägg i slutet.
import trimesh, numpy as np
from trimesh.creation import cylinder, box
import os
D=os.environ.get('PLUGG_DIR','.')   # läser D/in/k.stl, skriver till D/out/
os.makedirs(D+'/out',exist_ok=True)
U=lambda m: trimesh.boolean.union(m,engine='manifold')
Dif=lambda m: trimesh.boolean.difference(m,engine='manifold')
I=lambda m: trimesh.boolean.intersection(m,engine='manifold')
def cyl(r,z0,z1,x=0,y=0,sec=128):
    c=cylinder(radius=r,height=z1-z0,sections=sec); c.apply_translation([x,y,(z0+z1)/2]); return c
def cone(r0,r1,z0,z1,sec=128):
    a=np.linspace(0,2*np.pi,sec,endpoint=False)
    pts=np.vstack([np.c_[r0*np.cos(a),r0*np.sin(a),np.full(sec,z0)], np.c_[r1*np.cos(a),r1*np.sin(a),np.full(sec,z1)]])
    return trimesh.convex.convex_hull(pts)
def tube(ri,ro,z0,z1,sec=128): return Dif([cyl(ro,z0,z1,sec=sec), cyl(ri,z0-0.1,z1+0.1,sec=sec)])

PITCH=4.0
GRID=[(x,y) for x in (-PITCH,0,PITCH) for y in (-PITCH,0,PITCH)]

# ---------------- gänga (helix-svep av en trapetsprofil)
def thread(r_root, depth, pitch, z0, z1, flank=0.42, crest=0.12, n=48):
    turns=(z1-z0)/pitch; steps=int(turns*n)+1
    half_root=pitch*flank; half_crest=pitch*crest
    prof=[(r_root-0.3,-half_root),(r_root+depth,-half_crest),(r_root+depth,half_crest),(r_root-0.3,half_root)]
    verts=[]
    for i in range(steps):
        t=i/n*2*np.pi; z=z0+i/n*pitch
        for (r,dz) in prof: verts.append([r*np.cos(t), r*np.sin(t), z+dz])
    verts=np.array(verts); P=len(prof); faces=[]
    for i in range(steps-1):
        for j in range(P):
            a=i*P+j; b=i*P+(j+1)%P; c=(i+1)*P+(j+1)%P; d=(i+1)*P+j
            faces+= [[a,b,c],[a,c,d]]
    faces+= [[0,2,1],[0,3,2]]
    e=(steps-1)*P; faces+= [[e,e+1,e+2],[e,e+2,e+3]]
    m=trimesh.Trimesh(verts,faces,process=True); m.fix_normals()
    return m

# ---------------- uttaget och låsningens geometri
GAP        = 3.5     # glapp mellan uttagets framkant och flänsen när kontakten är helt intryckt (uppskattat från foto)
Z_SOCK     = 14.0 - GAP          # uttagets framkant i kontaktens koordinater
GROOVE_FAR = 6.0                 # framkant -> spårets bortre vägg i slutet (uppmätt)
Z_LUG_LOAD = Z_SOCK - GROOVE_FAR + 0.1   # klackens bärande yta (0,1 mm spel)
LUG_H      = 1.6                 # klackens höjd (spåret ca 2 mm)
R_NECK     = 20.6/2              # uttagets hals
R_LUG_TIP  = R_NECK - 1.0 + 0.15 # spårdjup 1 mm, 0,15 mm luft mot spårbotten

# ---------------- KÄRNA
src=trimesh.load(D+'/in/k.stl')
insert=I([src, box(extents=[40,40,14.0], transform=trimesh.transformations.translation_matrix([0,0,7.0]))])
# v2.2: ytterdiametern +0,15 mm. Insatsen skalas radiellt och bara det yttre skalet (r > 7,6 mm) läggs till,
# så stifthålen (r < 7,4 mm) inte påverkas.
OD_EXTRA = 0.15                       # mm på diametern
R_INS = 7.9                           # insatsens radie (uppmätt i k.stl, Ø15,8)
sc = (R_INS + OD_EXTRA/2) / R_INS
big = insert.copy(); big.apply_scale([sc, sc, 1.0])
skin = I([big, Dif([cyl(14, 0, 14.0), cyl(7.6, -1, 15)])])
insert = U([insert, skin])

R_FL=11.2                 # fläns Ø22,4 (låsringen går utanpå)
Z_FL0, Z_FL1 = 14.0, 16.0
R_COL=10.0                # krage som låsringens bakkant vilar på och som kåpan stoppar mot
Z_COL1 = 17.9
Z_TH0, Z_TH1 = Z_COL1, Z_COL1+10.0
R_CORE=8.9
TH_ROOT, TH_DEPTH, TH_PITCH = 8.9, 1.0, 2.0     # gängans ytterdiameter ca 19,8 mm, stigning 2

flange = U([cyl(R_FL, Z_FL0+0.6, Z_FL1), cone(R_FL-0.6, R_FL, Z_FL0, Z_FL0+0.6)])
collar = cyl(R_COL, Z_FL1-0.1, Z_COL1)
body   = cyl(R_CORE, Z_COL1-0.1, Z_TH1)
th     = thread(TH_ROOT, TH_DEPTH, TH_PITCH, Z_TH0+0.6, Z_TH1-1.4)
plugs  = [cyl(1.85, 1.9, 14.0, x, y, 40) for (x,y) in GRID]
core   = U([insert, flange, collar, body, th] + plugs)
core   = Dif([core, Dif([cyl(15,Z_TH1-0.8,Z_TH1+1), cone(R_CORE+0.2, R_CORE-0.6, Z_TH1-0.8, Z_TH1+0.01)])])
# sågtandsborrningar – v2.2: tunnlarna slutar 18 mm från spetsen (förut ända bak till gängans slut)
R_MIN, R_MAX, TOOTH = 1.55, 1.78, 1.2
TUNNEL = 18.0
Z_TEETH0, Z_TEETH1 = 2.0, TUNNEL
def sawtooth_bore(x, y, phase=0.0):
    pts=[(0, Z_TEETH0 - 0.6), (R_MIN, Z_TEETH0 - 0.6)]
    z=Z_TEETH0 + phase
    while z + TOOTH <= Z_TEETH1:
        pts += [(R_MIN, z), (R_MAX, z + TOOTH - 0.05), (R_MIN, z + TOOTH)]
        z += TOOTH
    pts += [(R_MIN, Z_TH1 + 1), (0, Z_TH1 + 1)]
    m = trimesh.creation.revolve(np.array(pts), sections=40)
    m.apply_translation([x, y, 0]); return m
bores=[sawtooth_bore(x, y, (TOOTH/2) if (round(x/PITCH)+round(y/PITCH))%2 else 0.0) for (x,y) in GRID]
# bakom tunnlarna: ihåligt rör (gängan sitter utanpå) + fas in i varje tunnel så hylsan hittar rätt
R_CAV = 7.45
def lead_in(x, y, r0=1.78, r1=2.15, h=0.7, n=24):
    a = np.linspace(0, 2*np.pi, n, endpoint=False)
    return trimesh.convex.convex_hull(np.vstack([np.c_[r0*np.cos(a)+x, r0*np.sin(a)+y, np.full(n, TUNNEL-h)],
                                                 np.c_[r1*np.cos(a)+x, r1*np.sin(a)+y, np.full(n, TUNNEL+0.01)]]))
cavity = U([cyl(R_CAV, TUNNEL, Z_TH1 + 1)] + [lead_in(x, y) for (x, y) in GRID])
bores = bores + [cavity]
# markering: pil i flänsens framsida på samma sida som styrklacken (+y)
mark=trimesh.convex.convex_hull(np.array([[-1.3,R_FL-0.2,Z_FL0-0.1],[1.3,R_FL-0.2,Z_FL0-0.1],[0,9.3,Z_FL0-0.1],
                                          [-1.3,R_FL-0.2,Z_FL0+0.6],[1.3,R_FL-0.2,Z_FL0+0.6],[0,9.3,Z_FL0+0.6]]))
# två axiella spår (vid ±x) så att låsringens klackar kan träs förbi gänga, krage och fläns vid montering
SLOT_W, SLOT_R = 2.3, 9.2
slots=[]
for ang in (0.0, np.pi):
    s=box(extents=[14.0-SLOT_R, SLOT_W, Z_TH1-Z_FL0+0.4])
    s.apply_translation([SLOT_R+(14.0-SLOT_R)/2, 0, (Z_FL0+Z_TH1)/2])
    s.apply_transform(trimesh.transformations.rotation_matrix(ang,[0,0,1])); slots.append(s)
core=Dif([core]+bores+[mark]+slots)
core.export(D+'/out/isobus_plugg_karna.stl')

# ---------------- LÅSRING
R_RING_IN, R_RING_OUT = R_FL+0.3, 13.0       # går fritt utanpå flänsen
Z_RING0 = Z_LUG_LOAD - 0.7                    # framkant (0,7 mm material framför klackarna)
Z_LIP0, Z_LIP1 = Z_FL1+0.1, Z_COL1-0.2        # bakre kant som vilar bakom flänsen
R_LIP_IN = R_COL+0.2
ring = U([tube(R_RING_IN, R_RING_OUT, Z_RING0, Z_LIP1), tube(R_LIP_IN, R_RING_OUT, Z_LIP0, Z_LIP1)])
# avfasad insida i framkanten (lättare att träffa uttaget)
ring = Dif([ring, cone(R_RING_IN+0.6, R_RING_IN-0.01, Z_RING0-0.01, Z_RING0+0.6)])
# två klackar mitt emot varandra: rund spets (passar spåret oavsett stigning) + avfasad rot (skrivs ut utan stöd)
lugs=[]
for ang in (0.0, np.pi):
    zc = Z_LUG_LOAD + LUG_H/2
    tip = cylinder(radius=LUG_H/2, height=(R_RING_IN+0.3)-R_LUG_TIP, sections=32)
    tip.apply_transform(trimesh.transformations.rotation_matrix(np.pi/2,[0,1,0]))
    tip.apply_translation([(R_LUG_TIP+R_RING_IN+0.3)/2, 0, zc])
    r0, r1 = R_NECK+0.25, R_RING_IN+0.3
    root = trimesh.convex.convex_hull(np.array([
        [r0,-0.8,Z_LUG_LOAD],[r0,0.8,Z_LUG_LOAD],[r0,-0.8,Z_LUG_LOAD+LUG_H],[r0,0.8,Z_LUG_LOAD+LUG_H],
        [r1,-1.2,Z_LUG_LOAD],[r1,1.2,Z_LUG_LOAD],[r1,-1.2,Z_LUG_LOAD+LUG_H+(r1-r0)],[r1,1.2,Z_LUG_LOAD+LUG_H+(r1-r0)]]))
    lug = U([tip, root])
    lug.apply_transform(trimesh.transformations.rotation_matrix(ang,[0,0,1]))
    lugs.append(lug)
ring = U([ring]+lugs)
# grepp: 18 räfflor + två små hack i framkanten som visar var klackarna sitter
cuts=[]
for k in range(18):
    a=k*2*np.pi/18 + np.pi/18
    cuts.append(cyl(0.8, Z_RING0+2.0, Z_LIP1-1.0, (R_RING_OUT+0.35)*np.cos(a), (R_RING_OUT+0.35)*np.sin(a), 24))
for ang in (0.0, np.pi):
    v=trimesh.convex.convex_hull(np.array([[R_RING_OUT+0.1,-0.9,Z_RING0-0.1],[R_RING_OUT+0.1,0.9,Z_RING0-0.1],
                                           [R_RING_OUT-0.7,0,Z_RING0-0.1],[R_RING_OUT+0.1,-0.9,Z_RING0+1.6],
                                           [R_RING_OUT+0.1,0.9,Z_RING0+1.6],[R_RING_OUT-0.7,0,Z_RING0+1.6]]))
    v.apply_transform(trimesh.transformations.rotation_matrix(ang,[0,0,1])); cuts.append(v)
ring = Dif([ring]+cuts)
ring.export(D+'/out/isobus_plugg_lasring.stl')

# ---------------- KÅPA (som v1, flyttad bakåt så att den stoppar mot kragen)
CLR=0.35
DZ = Z_COL1 - 17.0
Z_C0, Z_GRIP1, Z_TAPER1, Z_TOP = Z_COL1+0.02, 41.0+DZ, 52.0+DZ, 66.0+DZ
R_OUT=12.0
CABLE_R=3.6
outer = U([cyl(R_OUT, Z_C0+0.8, Z_GRIP1), cone(R_OUT-0.8, R_OUT, Z_C0, Z_C0+0.8),
           cone(R_OUT, 6.2, Z_GRIP1, Z_TAPER1), cyl(6.2, Z_TAPER1-0.1, Z_TOP-0.6), cone(6.2, 5.6, Z_TOP-0.6, Z_TOP)])
flutes=[]
for k in range(16):
    a=k*2*np.pi/16
    flutes.append(cyl(1.1, Z_C0+3, Z_GRIP1-2, (R_OUT+0.55)*np.cos(a), (R_OUT+0.55)*np.sin(a), 24))
rings=[Dif([cyl(8,z,z+1.0), cyl(5.5,z-0.1,z+1.1)]) for z in (55.0+DZ,58.0+DZ,61.0+DZ)]
outer=Dif([outer]+flutes+rings)
inner_th = thread(TH_ROOT+CLR, TH_DEPTH, TH_PITCH, Z_TH0+0.6-2*TH_PITCH, Z_TH1+3, flank=0.46, crest=0.16)
cav = U([cyl(R_CORE+CLR, Z_C0-1, Z_TH1+1.0), inner_th,
         cyl(8.0, Z_TH1+0.9, 44.0+DZ), cone(8.0, CABLE_R, 44.0+DZ, 48.5+DZ), cyl(CABLE_R, 48.4+DZ, Z_TOP+1)])
cap = Dif([outer, cav])
cap.export(D+'/out/isobus_plugg_kapa.stl')

asm=trimesh.util.concatenate([core, ring, cap]); asm.export(D+'/out/isobus_plugg_hopsatt.stl')
for n,mm in [('kärna',core),('låsring',ring),('kåpa',cap)]:
    print(n, mm.is_watertight, mm.bounds.round(2).tolist(), round(mm.volume/1000,2),'cm3')
# kollisionskontroll i monterat läge
for a,b,na in [(core,ring,'kärna/ring'),(core,cap,'kärna/kåpa'),(ring,cap,'ring/kåpa')]:
    x=I([a,b]); print('överlapp',na, round(x.volume,3) if len(x.faces) else 0.0,'mm3')
# monteringskontroll: låsringen träs på bakifrån – svep den från bakom gängan till sitt läge
worst=0.0
for dz in np.arange(0.0, Z_TH1-Z_LIP0+3.0, 0.5):
    r2=ring.copy(); r2.apply_translation([0,0,dz]); x=I([core,r2])
    worst=max(worst, x.volume if len(x.faces) else 0.0)
print('monteringssvep, största överlapp kärna/ring:', round(worst,3),'mm3')
