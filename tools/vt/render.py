# SPDX-License-Identifier: GPL-3.0-or-later
# Ritar en mask ur en ISO 11783-6 objektpool (förhandsvisning på datorn).
import sys, re, glob
from PIL import Image, ImageDraw, ImageFont
from pool import parse

def palette():
    P=[(0,0,0),(255,255,255),(0,153,0),(0,153,153),(153,0,0),(153,0,153),(153,153,0),(204,204,204),
       (153,153,153),(0,0,255),(0,255,0),(0,255,255),(255,0,0),(255,0,255),(255,255,0),(0,0,153)]
    for r in range(6):
        for g in range(6):
            for b in range(6): P.append((r*51,g*51,b*51))
    while len(P)<256: P.append((0,0,0))
    return P
PAL=palette()
FONTSZ={0:(6,8),1:(8,8),2:(8,12),3:(12,16),4:(16,16),5:(16,24),6:(24,32),7:(32,32),8:(32,48),9:(48,64),
        10:(64,64),11:(64,96),12:(96,128),13:(128,128),14:(128,192)}
TTF='/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf'
TTFB='/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf'
_fc={}
def ttf(h,bold):
    k=(h,bold)
    if k not in _fc: _fc[k]=ImageFont.truetype(TTFB if bold else TTF, max(6,int(h*1.05)))
    return _fc[k]

class VT:
    def __init__(s,d,S=1):
        s.O,_=parse(d); s.S=S
        ws=[o for o in s.O.values() if o['type']==0][0]
        s.ws=ws; s.mask=ws['mask']
    # ---------- kommandon från loggen (redskap -> VT, en ram)
    def cmd(s,b):
        f=b[0]; O=s.O
        g=lambda i: b[i]|b[i+1]<<8
        if f==0xA8:
            o=O.get(g(1))
            if o is not None: o['val']=b[4]|b[5]<<8|b[6]<<16|b[7]<<24
        elif f==0xA0:
            o=O.get(g(1))
            if o is not None and o['type']==3: o['hidden']=0 if b[3] else 1
        elif f==0xAD: s.mask=g(3)
        elif f==0xAE:
            o=O.get(g(2))
            if o is not None: o['skm']=g(4)
        elif f==0xA6:
            o=O.get(g(1))
            if o is not None: o['w']=g(3); o['h']=g(5)
        elif f==0xA7:
            o=O.get(g(1))
            if o is not None: o['bg']=b[3]
        elif f==0xA5:
            p=O.get(g(1))
            if p and 'kids' in p:
                c=g(3); p['kids']=[(i,x+b[5]-127,y+b[6]-127) if i==c else (i,x,y) for (i,x,y) in p['kids']]
        elif f==0xAF:
            o=O.get(g(1)); a=b[3]; v=b[4]|b[5]<<8|b[6]<<16|b[7]<<24
            if o is None: return
            # attribut-ID per typ (ISO 11783-6 bilaga): bara de vanligaste
            AT={14:{1:'la',2:'w',3:'h',4:'supp',5:'fa'},11:{1:'w',2:'h',3:'bg',4:'font',5:'opt',6:'var',7:'just'},
                12:{1:'w',2:'h',3:'bg',4:'font',5:'opt',6:'var',7:'off',8:'scale',9:'dec',10:'fmt',11:'just'},
                23:{1:'col',2:'size',3:'ftype',4:'style'},25:{1:'ftype',2:'col',3:'pat'},24:{1:'col',2:'lw',3:'art'},
                20:{1:'w',2:'opt',3:'tc'},6:{1:'w',2:'h',3:'bg',4:'border',5:'code',6:'opt'},3:{1:'w',2:'h'}}
            k=AT.get(o['type'],{}).get(a)
            if k: o[k]=v if k!='scale' else __import__('struct').unpack('<f',v.to_bytes(4,'little'))[0]
        elif f==0xB4:
            p=O.get(g(1))
            if p and 'kids' in p:
                c=g(3); x=g(5); y=g(7)
                p['kids']=[(i,x,y) if i==c else (i,a,bb) for (i,a,bb) in p['kids']]

    # ---------- ritning
    def col(s,i): return PAL[i&255]
    def text(s,im,x,y,w,h,txt,font,just,bg,transp,clip):
        fa=s.O.get(font,{'col':0,'size':0,'style':0})
        cw,ch=FONTSZ.get(fa.get('size',0),(6,8)); S=s.S
        dr=ImageDraw.Draw(im)
        if not transp: dr.rectangle([x*S,y*S,(x+w)*S-1,(y+h)*S-1],fill=s.col(bg))
        lines=txt.split('\n') if '\n' in txt else [txt]
        if fa.get('style',0)&0x10: pass
        tf=ttf(ch*S, fa.get('style',0)&1)
        hj=just&3; vj=(just>>2)&3
        tot=len(lines)*ch
        yy=y+(0 if vj==0 else (h-tot)//2 if vj==1 else h-tot)
        for ln in lines:
            ln=ln.rstrip() if hj else ln
            tw=len(ln)*cw
            xx=x+(0 if hj==0 else (w-tw)//2 if hj==1 else w-tw)
            for i,c in enumerate(ln):
                cx=xx+i*cw
                if cx+cw>x+w+0.5 or cx<x-0.5: continue
                if c==' ': continue
                dr.text(((cx+cw/2)*S,(yy+ch/2)*S),c,font=tf,fill=s.col(fa.get('col',0)),anchor='mm')
            yy+=ch
    def numtxt(s,o):
        raw=o['val']
        if o.get('var',0xFFFF)!=0xFFFF and o['var'] in s.O: raw=s.O[o['var']]['val']
        if raw>=2**31: raw-=2**32
        v=(raw+o['off'])*o['scale']
        if o['opt']&4 and v==0: return ''
        t=('%.*f'%(o['dec'],v)) if o['fmt']==0 else ('%.*e'%(o['dec'],v))
        return t
    def strval(s,o):
        v=o['val']
        if o.get('var',0xFFFF)!=0xFFFF and o['var'] in s.O: v=s.O[o['var']]['val']
        return v.decode('latin1').replace('\r','\n').replace('\x0a','\n')
    def picture(s,im,o,x,y):
        w,aw,ah=o['w'],o['aw'],o['ah']; fmt=o['fmt']; raw=o['raw']; S=s.S
        if o['opt']&4:
            out=bytearray(); i=0
            while i+1<len(raw): out+=bytes([raw[i+1]])*raw[i]; i+=2
            raw=bytes(out)
        bpp={0:1,1:4,2:8}[fmt]; rowb=(aw*bpp+7)//8
        px=Image.new('RGBA',(aw,ah))
        pd=px.load()
        for r in range(ah):
            for c in range(aw):
                bi=r*rowb+(c*bpp)//8
                if bi>=len(raw): continue
                b=raw[bi]
                if bpp==8: v=b
                elif bpp==4: v=(b>>4) if c%2==0 else (b&15)
                else: v=1 if (b>>(7-c%8))&1 else 0
                t=(o['opt']&1) and v==o['tc']
                pd[c,r]=(0,0,0,0) if t else PAL[v]+(255,)
        h=round(ah*w/aw) if aw else ah
        px=px.resize((w*S,h*S),Image.NEAREST)
        im.alpha_composite(px,(x*S,y*S))
    def draw(s,im,oid,x,y,depth=0):
        o=s.O.get(oid)
        if o is None or depth>20: return
        t=o['type']; S=s.S; dr=ImageDraw.Draw(im)
        if t in (1,2):
            dr.rectangle([x*S,y*S,(x+s.W)*S-1,(y+s.H)*S-1],fill=s.col(o['bg']))
            for (i,kx,ky) in o['kids']: s.draw(im,i,x+kx,y+ky,depth+1)
        elif t==3:
            if o['hidden']: return
            for (i,kx,ky) in o['kids']: s.draw(im,i,x+kx,y+ky,depth+1)
        elif t==27:
            if o['val']!=0xFFFF: s.draw(im,o['val'],x,y,depth+1)
        elif t in (5,):
            dr.rectangle([x*S,y*S,(x+s.KW)*S-1,(y+s.KH)*S-1],fill=s.col(o['bg']))
            for (i,kx,ky) in o['kids']: s.draw(im,i,x+kx,y+ky,depth+1)
        elif t==6:
            w,h=o['w'],o['h']
            dr.rectangle([x*S,y*S,(x+w)*S-1,(y+h)*S-1],fill=s.col(o['bg']))
            if not (o['opt']&0x20): dr.rectangle([x*S,y*S,(x+w)*S-1,(y+h)*S-1],outline=s.col(o['border']),width=max(1,S))
            for (i,kx,ky) in o['kids']: s.draw(im,i,x+kx+4,y+ky+4,depth+1)
        elif t==11:
            s.text(im,x,y,o['w'],o['h'],s.strval(o),o['font'],o['just'],o['bg'],o['opt']&1,True)
        elif t==8:
            s.text(im,x,y,o['w'],o['h'],s.strval(o),o['font'],o['just'],o['bg'],o['opt']&1,True)
        elif t in (12,9):
            s.text(im,x,y,o['w'],o['h'],s.numtxt(o),o['font'],o['just'],o['bg'],o['opt']&1,True)
        elif t==7:
            w=o['w']; fa=s.O.get(o['fg'],{'col':0})
            dr.rectangle([x*S,y*S,(x+w)*S-1,(y+w)*S-1],fill=s.col(o['bg']),outline=s.col(fa.get('col',0)))
            v=o['val']
            if o['var']!=0xFFFF and o['var'] in s.O: v=s.O[o['var']]['val']
            if v: dr.line([(x+2)*S,(y+w/2)*S,(x+w/2)*S,(y+w-3)*S,(x+w-2)*S,(y+2)*S],fill=s.col(fa.get('col',0)),width=2*S)
        elif t in (10,37):
            v=o['val']
            if o['var']!=0xFFFF and o['var'] in s.O: v=s.O[o['var']]['val']
            if v<len(o['items']) and o['items'][v]!=0xFFFF: s.draw(im,o['items'][v],x,y,depth+1)
        elif t==14:
            la=s.O.get(o['la'],{'col':0,'lw':1}); fa=s.O.get(o['fa'])
            box=[x*S,y*S,(x+o['w'])*S-1,(y+o['h'])*S-1]
            fill=None
            if fa:
                if fa['ftype']==1: fill=s.col(la['col'])
                elif fa['ftype'] in (2,3): fill=s.col(fa['col'])
            if fill: dr.rectangle(box,fill=fill)
            if la.get('lw',1) and o['supp']!=15: dr.rectangle(box,outline=s.col(la['col']),width=la['lw']*S)
        elif t==15:
            la=s.O.get(o['la'],{'col':0,'lw':1}); fa=s.O.get(o['fa'])
            box=[x*S,y*S,(x+o['w'])*S-1,(y+o['h'])*S-1]
            fill=s.col(fa['col']) if fa and fa['ftype'] in (2,3) else (s.col(la['col']) if fa and fa['ftype']==1 else None)
            dr.ellipse(box,fill=fill,outline=s.col(la['col']) if la.get('lw',1) else None,width=max(1,la.get('lw',1))*S)
        elif t==16:
            la=s.O.get(o['la'],{'col':0,'lw':1}); fa=s.O.get(o['fa'])
            pts=[((x+px)*S,(y+py)*S) for px,py in o['pts']]
            fill=s.col(fa['col']) if fa and fa['ftype'] in (2,3) else (s.col(la['col']) if fa and fa['ftype']==1 else None)
            if fill and len(pts)>2: dr.polygon(pts,fill=fill)
            if la.get('lw',1): dr.line(pts+([pts[0]] if o['ptype']!=3 else []),fill=s.col(la['col']),width=la['lw']*S)
        elif t==13:
            la=s.O.get(o['la'],{'col':0,'lw':1})
            p=[(x*S,y*S),((x+o['w'])*S,(y+o['h'])*S)] if o['dir']==0 else [(x*S,(y+o['h'])*S),((x+o['w'])*S,y*S)]
            dr.line(p,fill=s.col(la['col']),width=max(1,la['lw'])*S)
        elif t==20:
            s.picture(im,o,x,y)
    def render(s,W,H,KW,KH,nkeys):
        s.W,s.H,s.KW,s.KH=W,H,KW,KH; S=s.S
        cols=2; per=nkeys//cols
        GAP=4
        im=Image.new('RGBA',((W+GAP+cols*(KW+GAP))*S,max(H,per*(KH+GAP))*S),(40,40,40,255))
        m=s.O[s.mask]
        mi=Image.new('RGBA',(W*S,H*S),(0,0,0,255)); s.draw(mi,s.mask,0,0); im.alpha_composite(mi,(0,0))
        skm=s.O.get(m.get('skm',0xFFFF))
        keys=skm['keys'] if skm else []
        for k in range(nkeys):
            c=k//per; r=k%per
            kx=W+GAP+c*(KW+GAP); ky=r*(KH+GAP)
            ki=Image.new('RGBA',(KW*S,KH*S),(0,0,0,255))
            ImageDraw.Draw(ki).rectangle([0,0,KW*S-1,KH*S-1],fill=PAL[skm['bg']] if skm else (60,60,60))
            if k<len(keys) and keys[k]!=0xFFFF: s.draw(ki,keys[k],0,0)
            im.alpha_composite(ki,(kx*S,ky*S))
        return im

def replay(vt,logs,until=None):
    n=0
    for fn in logs:
        for ln in open(fn):
            m=re.match(r'\((\d+\.\d+)\) can0 ([0-9A-F]{8})#([0-9A-F]+)',ln)
            if not m: continue
            t=float(m.group(1)); i=int(m.group(2),16)
            if until and t>until: return n
            pf=(i>>16)&0xFF; da=(i>>8)&0xFF; sa=i&0xFF
            if pf==0xE7 and sa==0x82 and da==0x81:
                vt.cmd(bytes.fromhex(m.group(3))); n+=1
    return n

if __name__=='__main__':
    d=open(sys.argv[1],'rb').read()
    vt=VT(d,S=3)
    logs=sorted(glob.glob(sys.argv[2])) if len(sys.argv)>2 else []
    n=replay(vt,logs)
    print('kommandon',n,'aktiv mask',vt.mask)
    im=vt.render(280,280,80,52,12)
    im.convert('RGB').save(sys.argv[3] if len(sys.argv)>3 else '/tmp/vt/mask.png')
