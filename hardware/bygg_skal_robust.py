# SPDX-License-Identifier: CERN-OHL-S-2.0
# Robust skal för Waveshare ESP32-S3-Touch-LCD-7 (egen konstruktion, v2)
#
# v2: skärmen låg lös – den hölls bara av de fyra M3-skruvarna i skärmens insatser.
#   Nu trycker fyra pressklackar i baklocket glaset ända fram mot framkanten (ingen skumlist):
#   glaset ligger z = -1 .. +1, klackarna slutar 0,15 mm in i glaset (förspänning).
#   Fästena mot M3-insatserna är 1 mm längre (insatserna flyttar 1 mm framåt).
#   Framskalet är oförändrat; bara baklocket ska skrivas om.
#
# Delar:  framskal (kant runt glaset + väggar), baklock (RAM-hålbild, fästen för kabelklämmor),
#         2 kabelklämmor, 1 blindplugg (stänger USB-kabelns hål när 12 V-matning byggs in).
# Koordinater: sett FRAMIFRÅN, x åt höger, y uppåt, origo = glasets mitt.
#              z = 0 vid glasets framsida, z ökar BAKÅT.
# Mått från Waveshares ritning: glas 192,96 x 110,76, bakplåt 165,72 x 97,60,
# M3-insatser 126,20 x 65,65. Insatsernas topp 17,0 mm bakom glasets framsida
# och M3-hålens läge är kontrollerade mot ett skal som är byggt för kortet.
import trimesh, numpy as np
from shapely.geometry import Polygon, box as sbox, Point
from shapely.ops import unary_union
from shapely import affinity
import os
OUT=os.environ.get('SKAL_OUT','out')   # utmapp för STL-filerna
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
def stadium(cx,cy,half,r):   # avlångt hål i x-led
    return sbox(cx-half,cy-0.001,cx+half,cy+0.001).buffer(r,resolution=24)

# ---------------- mått
GW, GH   = 192.96, 110.76
CL       = 0.3
PW, PH   = GW+2*CL, GH+2*CL          # glasficka 193,56 x 111,36
R_POCK   = 6.0
WALL     = 5.0                        # tjock vägg: lockets skruvar går i väggen (inget får sticka in i glasets väg)
OW, OH   = PW+2*WALL, PH+2*WALL      # 203,56 x 121,36
R_OUT    = R_POCK+WALL
Z_LIPF, Z_LIPB = -3.5, -1.0          # kantens fram-/baksida (1 mm luft till glaset för skumlist)
LIP_IN   = 3.2                        # kanten täcker glasets svarta ram 3,2 mm
Z_GLASS  = -1.0                       # glaset trycks fram mot kanten: glasets framsida z = -1 (v2)
GT       = 2.0                        # glasets tjocklek
Z_INS    = 17.0 + Z_GLASS             # M3-insatsernas topp (17,0 mm bakom glasets framsida)
PRESS_PRE= 0.15                       # klackarnas förspänning mot glaset
Z_BACK   = 23.0                       # baklockets insida (6 mm extra plats för kablar)
T_COVER  = 4.0
Z_END    = Z_BACK+T_COVER
CH       = 2.5                        # fasade ytterkanter
XI, YI   = PW/2, PH/2                 # väggarnas insida: x ±96,78, y ±55,68

# M3-insatser: y säker; x mellan ritningen (±63,1) och referensskalet (+1,5 mm) -> avlånga hål
INS_X = [-63.1+0.4, 63.1+0.4]; INS_Y = [-32.40, 33.25]; INS_SLOT = 1.6
# kortets kontakter (framifrån): UART1 USB-C vid x≈+57, y≈+2 (pekar åt +x); CAN-kontakt vid x≈-49, y≈+7 (pekar åt -x)
USB_X, USB_W = 75.0, 5.4              # kabelhål i nederkant för USB-kabeln
ISO_X, ISO_W = -70.0, 7.6             # kabelhål i nederkant för ISOBUS-kabeln (kåpans kabelhål är Ø7,2)
CLAMP_Y = -YI+9.5
# DC/DC EKM006 (60 x 20 mm) – står på högkant i två spår i baklocket, längs vänster vägg (sett framifrån)
DC_Y0, DC_Z0 = 5.0, 13.0
DC_X0 = -XI+2.0                       # kortets baksida (1,6 mm tjockt, komponenterna inåt)
# lockets skruvar går rakt in i väggarna (ovan- och nederkant), utanför glasets väg
WALL_SCREWS = [(x, YI+WALL/2) for x in (-45,45)] + [(x, -YI-WALL/2) for x in (-40,40)]
RAM = [(sx*32, sy*16) for sx in (-1,1) for sy in (-1,1)]

# ================= FRAMSKAL
outer = U([hull_rr(OW-2*CH,OH-2*CH,R_OUT-CH,Z_LIPF, OW,OH,R_OUT,Z_LIPF+CH),
           ext(rrect(OW,OH,R_OUT), Z_LIPF+CH-0.01, Z_BACK-0.6),
           hull_rr(OW,OH,R_OUT,Z_BACK-0.61, OW-1.2,OH-1.2,R_OUT-0.6,Z_BACK)])
cav   = ext(rrect(PW,PH,R_POCK), Z_LIPB, Z_BACK+1)
win   = ext(rrect(GW-2*LIP_IN, GH-2*LIP_IN, 3.0), Z_LIPF-1, Z_LIPB+0.1)
winch = hull_rr(GW-2*LIP_IN+3.0, GH-2*LIP_IN+3.0, 4.5, Z_LIPF-0.01, GW-2*LIP_IN, GH-2*LIP_IN, 3.0, Z_LIPF+1.5)  # fasad öppning
shell = Dif([outer, cav, win, winch])
# skruvhål för baklocket i väggarna
holes=[cylz(1.25, Z_BACK-12, Z_BACK+1, x, y) for (x,y) in WALL_SCREWS]
# kabelhål i nederkanten: U-spår öppet bakåt, baklocket stänger ovanifrån
def notch(x,w):
    p=unary_union([Point(0,0).buffer(w/2,resolution=24), sbox(-w/2,0,w/2,20)])
    m=trimesh.creation.extrude_polygon(p, WALL+4)
    # polygonens (u,v) -> (x, z), extrudering längs y
    M=np.eye(4); M[:3,0]=[1,0,0]; M[:3,1]=[0,0,1]; M[:3,2]=[0,-1,0]; M[:3,3]=[x,-YI+1,Z_BACK-w/2]
    m.apply_transform(M); return m
holes += [notch(USB_X,USB_W), notch(ISO_X,ISO_W)]
shell = Dif([shell]+holes)

# ================= BAKLOCK
cover = U([ext(rrect(OW,OH,R_OUT), Z_BACK, Z_END-CH),
           hull_rr(OW,OH,R_OUT,Z_END-CH-0.01, OW-2*CH,OH-2*CH,R_OUT-CH,Z_END)])
# inre kant som styr locket i skalet (avbruten vid hörn och kabelhål)
rim_o = rrect(PW-0.6,PH-0.6,R_POCK-0.3); rim_i = rrect(PW-3.6,PH-3.6,R_POCK-1.8)
rim = rim_o.difference(rim_i)
for x,w in ((USB_X,USB_W),(ISO_X,ISO_W)): rim = rim.difference(sbox(x-w/2-1.5,-YI-5,x+w/2+1.5,-YI+5))
rim = rim.difference(sbox(-XI-5, DC_Y0-31, -XI+5, DC_Y0+31))     # fritt för DC/DC-kortets baksida
parts=[cover, ext(rim, Z_BACK-2.0, Z_BACK+0.01)]
# tungor som fyller kabelhålens raka del uppifrån (så att hålet blir runt)
for x,w in ((USB_X,USB_W),(ISO_X,ISO_W)):
    t=sbox(x-w/2+0.25, -YI-WALL+0.3, x+w/2-0.25, -YI-0.3)
    parts.append(ext(t, Z_BACK-w/2+0.0001, Z_BACK+0.01))
# PRESSKLACKAR (v2): trycker på glasets kant utanför skärmens bakplåt, i alla fyra hörn.
# Glaskanten är fri för x > 84,4 (bakplåten 165,7 mm bred + 1,5 mm osäkerhet). På vänster sida sitter
# DC/DC-kortet mellan y = -26 och +36, så klackarna står vid y = ±44.
PRESS = [(sx*89.5, sy*44.0) for sx in (-1,1) for sy in (-1,1)]
Z_PRESS = Z_GLASS + GT - PRESS_PRE
for (x,y) in PRESS:
    parts.append(U([cylz(3.0, Z_PRESS+1.0, Z_BACK+0.01, x, y),
                    trimesh.convex.convex_hull(np.vstack([cylz(2.2, Z_PRESS, Z_PRESS+0.01, x, y).vertices,
                                                          cylz(3.0, Z_PRESS+1.0, Z_PRESS+1.01, x, y).vertices]))]))
# fästen mot skärmens M3-insatser (7 mm långa i v2)
for x in INS_X:
    for y in INS_Y: parts.append(ext(stadium(x,y,INS_SLOT,4.2), Z_INS+0.05, Z_BACK+0.01))
# spår som håller DC/DC-kortet (skjuts ner i spåren, kortändarna fångas 1,5 mm)
for sgn in (-1,1):
    ye = DC_Y0 + sgn*30.0
    blk = ext(sbox(-XI+0.4, min(ye-sgn*1.5, ye+sgn*3.5), -XI+6.0, max(ye-sgn*1.5, ye+sgn*3.5)), 3.0, Z_BACK+0.01)
    slot= ext(sbox(DC_X0-0.2, min(ye-sgn*1.6, ye+sgn*0.4), DC_X0+1.6+0.2, max(ye-sgn*1.6, ye+sgn*0.4)), 2.0, Z_BACK-0.5)
    parts.append(Dif([blk, slot]))
# fästen för kabelklämmorna
CLAMP = [(USB_X, 7.0, 2.5), (ISO_X, 8.5, 3.5)]     # (x, halvt skruvavstånd, höjd) – klämmer kablar från ca 3,4 resp. 4,9 mm
for (x,hs,hh) in CLAMP:
    for s in (-1,1): parts.append(cylz(3.2, Z_BACK-hh, Z_BACK+0.01, x+s*hs, CLAMP_Y))
cover = U(parts)
# tungorna: rund botten = skär bort en cylinder längs y
for x,w in ((USB_X,USB_W),(ISO_X,ISO_W)):
    c=trimesh.creation.cylinder(radius=w/2, height=WALL+2, sections=32)
    c.apply_transform(trimesh.transformations.rotation_matrix(np.pi/2,[1,0,0])); c.apply_translation([x,-YI-WALL/2,Z_BACK-w/2])
    cover=Dif([cover,c])
ch=[]
for (x,y) in WALL_SCREWS:
    ch += [cylz(1.7, Z_BACK-1, Z_END+1, x, y), cylz(3.3, Z_END-2.2, Z_END+1, x, y)]
for x in INS_X:
    for y in INS_Y:
        ch += [ext(stadium(x,y,INS_SLOT,1.75), Z_INS-1, Z_END+1), ext(stadium(x,y,INS_SLOT,3.3), Z_END-2.2, Z_END+1)]
for (x,y) in RAM: ch.append(cylz(2.2, Z_BACK-1, Z_END+1, x, y))
for (x,hs,hh) in CLAMP:
    for s in (-1,1): ch.append(cylz(1.25, Z_BACK-hh-1, Z_BACK+2.5, x+s*hs, CLAMP_Y))
# spår i utsidan (robust mönster), RAM-ytan lämnas slät
g=[]
for t in (rrect(OW-14,OH-14,R_OUT-5).exterior.buffer(1.25),):
    g.append(t)
for x in (-62,62): g.append(sbox(x-1.25,-OH/2+7,x+1.25,OH/2-7))
for y in (-30,30): g.append(sbox(-OW/2+7,y-1.25,OW/2-7,y+1.25))
grooves = unary_union(g).difference(rrect(88,56,6)).intersection(rrect(OW-12,OH-12,R_OUT-4))
ch.append(ext(grooves, Z_END-1.5, Z_END+1))
cover = Dif([cover]+ch)

# ================= KABELKLÄMMOR (skrivs ut platt)
def clamp(hs, d_max, name):
    L=2*hs+9; bar=ext(rrect(L,8,2.5),0,4.0)
    v=trimesh.convex.convex_hull(np.array([[-d_max*0.6,-5,-0.01],[d_max*0.6,-5,-0.01],[0,-5,d_max*0.2],
                                           [-d_max*0.6,5,-0.01],[d_max*0.6,5,-0.01],[0,5,d_max*0.2]]))   # grunt V-spår
    hl=[cylz(1.7,-1,5,s*hs,0) for s in (-1,1)]
    m=Dif([bar,v]+hl); m.export(OUT+f'/{name}.stl'); return m
clamp(7.0, 5.0, 'skal7_klamma_usb'); clamp(8.5, 7.5, 'skal7_klamma_isobus')
# blindplugg för USB-hålet (när DC/DC byggs in)
p2 = Point(0,0).buffer(USB_W/2-0.15,resolution=32)
plug = U([ext(sbox(-USB_W/2+0.15,0,USB_W/2-0.15,USB_W/2), 0, WALL-0.2),
          ext(p2, 0, WALL-0.2),
          ext(sbox(-USB_W/2-1.2,-USB_W/2-1.0,USB_W/2+1.2,USB_W/2), WALL-0.2, WALL+1.0)])   # fläns på insidan
plug.export(OUT+'/skal7_blindplugg_usb.stl')

# ================= STÖTSKYDD I TPU (egen del i samma koordinater -> eget filament i Bambu Studio)
# Hörnskydd som går runt hörnen och 1 mm förbi framsidan (skärmen vilar på TPU om den läggs med glaset nedåt),
# plus en dyna mitt på varje kortsida. TPU:n ligger 1,5 mm nedsänkt i väggen och sticker ut 1,5 mm.
BUMP_OUT, BUMP_IN, Z_B0, Z_B1, Z_BF = 1.5, 1.5, -1.0, 21.0, Z_LIPF-1.0
outer2d = rrect(OW,OH,R_OUT)
SX, SY = OW/2-R_OUT, OH/2-R_OUT                 # där de raka sidorna börjar (90,78 / 49,68)
zones=[]
for sx in (-1,1):
    for sy in (-1,1):
        if sy>0: Lx=16.0
        else:    Lx = 11.0 if sx>0 else 15.0         # håll fritt från kabelhålen i nederkanten
        Ly=16.0
        zones.append(sbox(min(sx*(SX-Lx), sx*200), min(sy*(SY-Ly), sy*200), max(sx*(SX-Lx), sx*200), max(sy*(SY-Ly), sy*200)))
corner_zone = unary_union(zones)
side_zone = unary_union([sbox(-300,-16,-SX+5,16), sbox(SX-5,-16,300,16)])
env = U([hull_rr(OW+2*BUMP_OUT-2.0, OH+2*BUMP_OUT-2.0, R_OUT+BUMP_OUT-1.0, Z_BF,
                 OW+2*BUMP_OUT, OH+2*BUMP_OUT, R_OUT+BUMP_OUT, Z_BF+1.0),
         ext(rrect(OW+2*BUMP_OUT, OH+2*BUMP_OUT, R_OUT+BUMP_OUT), Z_BF+0.99, Z_B1-1.5),
         hull_rr(OW+2*BUMP_OUT, OH+2*BUMP_OUT, R_OUT+BUMP_OUT, Z_B1-1.51, OW, OH, R_OUT, Z_B1)])
core_side  = ext(outer2d.buffer(-BUMP_IN), Z_B0, Z_B1+1)
core_front = ext(outer2d.buffer(-5.0), Z_BF-1, Z_B0+0.01)
cz = ext(corner_zone, Z_BF-1, Z_B1+1)
sz = ext(side_zone, Z_B0, Z_B1+1)
bump_c = Dif([I([env, cz]), core_side, core_front, ext(outer2d.buffer(-BUMP_IN), Z_B0-0.01, Z_B1+1)])
bump_s = Dif([I([env, sz, ext(outer2d.buffer(10), Z_B0, Z_B1+1)]), core_side])
bumpers = U([bump_c, bump_s])
shell = Dif([shell, bumpers])
bumpers.export(OUT+'/skal7_framskal_TPU.stl')
print('TPU-delar', bumpers.is_watertight, round(bumpers.volume/1000,1),'cm3', len(bumpers.split(only_watertight=False)),'st')

shell.export(OUT+'/skal7_framskal.stl'); cover.export(OUT+'/skal7_baklock.stl')

# ================= kontroller
print('framskal', shell.is_watertight, shell.bounds.round(2).tolist(), round(shell.volume/1000,1),'cm3')
print('baklock ', cover.is_watertight, cover.bounds.round(2).tolist(), round(cover.volume/1000,1),'cm3')
x=I([shell,cover]); print('överlapp skal/lock', round(x.volume,3) if len(x.faces) else 0)
# glaset + skärmmodulen som provkropp
glass = ext(rrect(GW,GH,7.0), Z_GLASS, Z_GLASS+GT)
bp_x0 = -165.72/2; bp_y1 = GH/2-3.8
module = ext(sbox(-165.72/2-1.5, bp_y1-97.6, 165.72/2+1.5, bp_y1), Z_GLASS+GT, 12.5+Z_GLASS)     # bakplåt, ±1,5 mm osäkerhet i x
ins = [cylz(2.5, 12.5+Z_GLASS, Z_INS, x+dx, y) for x in (-63.1,63.1) for y in INS_Y for dx in (0,)]
pcb = ext(sbox(-49.1-6, -43.1, 56.9+1, 25.0+3), 12.5+Z_GLASS, Z_INS-0.2)   # kretskort + komponenter (från ritningen)
dcdc=trimesh.creation.box(extents=[1.6,59.4,19.6]); dcdc.apply_translation([DC_X0+0.8, DC_Y0, Z_BACK-0.5-9.8])            # DC/DC-kortet
dcdc_k=trimesh.creation.box(extents=[8.0,52,18]); dcdc_k.apply_translation([DC_X0+1.6+4.0, DC_Y0, Z_BACK-0.5-9.8])        # dess komponenter
for nm,b in [('glas',glass),('bakplåt',module),('kort',pcb),('DC/DC',dcdc),('DC/DC-komp',dcdc_k)]+[('insats',i) for i in ins]:
    for pn,p in [('skal',shell),('lock',cover)]:
        x=I([b,p]); v=x.volume if len(x.faces) else 0
        if v>0.01: print('KROCK' if not (nm=='glas' and pn=='lock') else 'förspänning (avsiktlig)', nm, pn, round(v,2))
# MONTERING: skärmen (glas + bakplåt + kort) skjuts in bakifrån i framskalet
mod_all = U([glass, module, pcb] + ins)
stuck=[]
for dz in np.arange(Z_BACK+12, -0.01, -0.5):
    m=mod_all.copy(); m.apply_translation([0,0,dz]); x=I([shell,m])
    if len(x.faces) and x.volume>0.01: stuck.append(round(float(dz),1))
print('montering skärm i framskal:', 'OK' if not stuck else f'FASTNAR vid förskjutning {stuck[:5]}')
# MONTERING: locket sätts rakt på bakifrån (med DC/DC i spåren och skärmen på plats)
stuck=[]
for dz in np.arange(15, -0.01, -0.5):
    c=cover.copy(); c.apply_translation([0,0,dz]); x=I([U([shell,mod_all]),c])
    if len(x.faces) and x.volume>0.01: stuck.append(round(float(dz),1))
print('montering lock:', 'OK' if not stuck else ('OK (klackarna tar i glaset först i slutläget = förspänning)' if stuck==[0.0] else f'FASTNAR vid {stuck[:5]}'))
print('kontroll klar')
asm=trimesh.util.concatenate([shell,cover]); asm.export(OUT+'/skal7_hopsatt.stl')
