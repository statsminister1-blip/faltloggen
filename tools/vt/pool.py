# SPDX-License-Identifier: GPL-3.0-or-later
# Tolkar en ISO 11783-6 objektpool (VT v3/v4). Returnerar dict id -> objekt.
import struct
def u8(d,p): return d[p],p+1
def u16(d,p): return d[p]|d[p+1]<<8,p+2
def u32(d,p): return struct.unpack_from('<I',d,p)[0],p+4
def i32(d,p): return struct.unpack_from('<i',d,p)[0],p+4
def f32(d,p): return struct.unpack_from('<f',d,p)[0],p+4
NAMES={0:'WorkingSet',1:'DataMask',2:'AlarmMask',3:'Container',4:'SoftKeyMask',5:'Key',6:'Button',7:'InputBool',
 8:'InputString',9:'InputNumber',10:'InputList',11:'OutputString',12:'OutputNumber',13:'Line',14:'Rect',15:'Ellipse',
 16:'Polygon',17:'Meter',18:'LinearBar',19:'ArchedBar',20:'Picture',21:'NumVar',22:'StrVar',23:'FontAttr',24:'LineAttr',
 25:'FillAttr',26:'InputAttr',27:'ObjPointer',28:'Macro',29:'AuxFunc',30:'AuxInput',31:'AuxFunc2',32:'AuxInput2',33:'AuxCtrlPtr',
 34:'WindowMask',35:'KeyGroup',36:'GraphicsContext',37:'OutputList',38:'ExtInputAttr',39:'ColourMap',40:'ObjLabelRefList'}
def parse(d):
    objs={}; p=0; order=[]
    while p<len(d):
        start=p
        oid,p=u16(d,p); t,p=u8(d,p); o={'id':oid,'type':t,'tn':NAMES.get(t,'?%d'%t),'pos':start}
        def F(name,fn):
            nonlocal p
            v,p=fn(d,p); o[name]=v; return v
        def kids(n):  # (id,x,y)
            nonlocal p
            l=[]
            for _ in range(n):
                i,p=u16(d,p); x,p=u16(d,p); y,p=u16(d,p)
                if x>=32768: x-=65536
                if y>=32768: y-=65536
                l.append((i,x,y))
            return l
        def ids(n):
            nonlocal p
            l=[]
            for _ in range(n): i,p=u16(d,p); l.append(i)
            return l
        def macros(n):
            nonlocal p; p+=2*n
        if t==0:
            F('bg',u8);F('sel',u8);F('mask',u16);no=F('n',u8);nm=F('nm',u8);nl=F('nl',u8)
            o['kids']=kids(no);macros(nm);o['lang']=[d[p+2*i:p+2*i+2].decode('latin1') for i in range(nl)];p+=2*nl
        elif t in (1,):
            F('bg',u8);F('skm',u16);no=F('n',u8);nm=F('nm',u8);o['kids']=kids(no);macros(nm)
        elif t==2:
            F('bg',u8);F('skm',u16);F('prio',u8);F('ac',u8);no=F('n',u8);nm=F('nm',u8);o['kids']=kids(no);macros(nm)
        elif t==3:
            F('w',u16);F('h',u16);F('hidden',u8);no=F('n',u8);nm=F('nm',u8);o['kids']=kids(no);macros(nm)
        elif t==4:
            F('bg',u8);no=F('n',u8);nm=F('nm',u8);o['keys']=ids(no);macros(nm)
        elif t==5:
            F('bg',u8);F('code',u8);no=F('n',u8);nm=F('nm',u8);o['kids']=kids(no);macros(nm)
        elif t==6:
            F('w',u16);F('h',u16);F('bg',u8);F('border',u8);F('code',u8);F('opt',u8);no=F('n',u8);nm=F('nm',u8);o['kids']=kids(no);macros(nm)
        elif t==7:
            F('bg',u8);F('w',u16);F('fg',u16);F('var',u16);F('val',u8);F('en',u8);nm=F('nm',u8);macros(nm)
        elif t==8:
            F('w',u16);F('h',u16);F('bg',u8);F('font',u16);F('inattr',u16);F('opt',u8);F('var',u16);F('just',u8);n=F('len',u8)
            o['val']=d[p:p+n];p+=n;F('en',u8);nm=F('nm',u8);macros(nm)
        elif t==9:
            F('w',u16);F('h',u16);F('bg',u8);F('font',u16);F('opt',u8);F('var',u16);F('val',u32);F('min',i32);F('max',i32);F('off',i32);F('scale',f32)
            F('dec',u8);F('fmt',u8);F('just',u8);F('opt2',u8);nm=F('nm',u8);macros(nm)
        elif t==10:
            F('w',u16);F('h',u16);F('var',u16);F('val',u8);ni=F('ni',u8);F('opt',u8);nm=F('nm',u8);o['items']=ids(ni);macros(nm)
        elif t==11:
            F('w',u16);F('h',u16);F('bg',u8);F('font',u16);F('opt',u8);F('var',u16);F('just',u8);n=F('len',u16)
            o['val']=d[p:p+n];p+=n;nm=F('nm',u8);macros(nm)
        elif t==12:
            F('w',u16);F('h',u16);F('bg',u8);F('font',u16);F('opt',u8);F('var',u16);F('val',u32);F('off',i32);F('scale',f32)
            F('dec',u8);F('fmt',u8);F('just',u8);nm=F('nm',u8);macros(nm)
        elif t==13:
            F('la',u16);F('w',u16);F('h',u16);F('dir',u8);nm=F('nm',u8);macros(nm)
        elif t==14:
            F('la',u16);F('w',u16);F('h',u16);F('supp',u8);F('fa',u16);nm=F('nm',u8);macros(nm)
        elif t==15:
            F('la',u16);F('w',u16);F('h',u16);F('etype',u8);F('a0',u8);F('a1',u8);F('fa',u16);nm=F('nm',u8);macros(nm)
        elif t==16:
            F('w',u16);F('h',u16);F('la',u16);F('fa',u16);F('ptype',u8);n=F('np',u8);nm=F('nm',u8)
            o['pts']=[(d[p+4*i]|d[p+4*i+1]<<8, d[p+4*i+2]|d[p+4*i+3]<<8) for i in range(n)];p+=4*n;macros(nm)
        elif t==17:
            F('w',u16);F('needle',u8);F('border',u8);F('arc',u8);F('opt',u8);F('ticks',u8);F('a0',u8);F('a1',u8);F('min',u16);F('max',u16);F('var',u16);F('val',u16);nm=F('nm',u8);macros(nm)
        elif t==18:
            F('w',u16);F('h',u16);F('col',u8);F('tcol',u8);F('opt',u8);F('ticks',u8);F('min',u16);F('max',u16);F('var',u16);F('val',u16);F('tvar',u16);F('tval',u16);nm=F('nm',u8);macros(nm)
        elif t==19:
            F('w',u16);F('h',u16);F('col',u8);F('tcol',u8);F('opt',u8);F('a0',u8);F('a1',u8);F('bw',u16);F('min',u16);F('max',u16);F('var',u16);F('val',u16);F('tvar',u16);F('tval',u16);nm=F('nm',u8);macros(nm)
        elif t==20:
            F('w',u16);F('aw',u16);F('ah',u16);F('fmt',u8);F('opt',u8);F('tc',u8);n=F('nraw',u32);nm=F('nm',u8)
            o['raw']=d[p:p+n];p+=n;macros(nm)
        elif t==21: F('val',u32)
        elif t==22:
            n=F('len',u16);o['val']=d[p:p+n];p+=n
        elif t==23:
            F('col',u8);F('size',u8);F('ftype',u8);F('style',u8);nm=F('nm',u8);macros(nm)
        elif t==24:
            F('col',u8);F('lw',u8);F('art',u16);nm=F('nm',u8);macros(nm)
        elif t==25:
            F('ftype',u8);F('col',u8);F('pat',u16);nm=F('nm',u8);macros(nm)
        elif t==26:
            F('vtype',u8);n=F('len',u8);o['vstr']=d[p:p+n];p+=n;nm=F('nm',u8);macros(nm)
        elif t==27: F('val',u16)
        elif t==28:
            n=F('nb',u16);o['cmd']=d[p:p+n];p+=n
        elif t in (29,31,32):
            F('bg',u8);F('fa',u8);no=F('n',u8);o['kids']=kids(no)
        elif t==30:
            F('bg',u8);F('fa',u8);F('inid',u8);no=F('n',u8);o['kids']=kids(no)
        elif t==33:
            F('ptype',u8);F('val',u16)
        elif t==37:
            F('w',u16);F('h',u16);F('var',u16);F('val',u8);ni=F('ni',u8);nm=F('nm',u8);o['items']=ids(ni);macros(nm)
        elif t==39:
            n=F('n',u16);o['map']=d[p:p+n];p+=n
        elif t==40:
            n=F('n',u16);p+=7*n
        elif t==35:
            F('opt',u8);F('name',u16);F('icon',u16);no=F('n',u8);nm=F('nm',u8);o['keys']=ids(no);macros(nm)
        else:
            raise ValueError('okänd typ %d id %d vid %d'%(t,oid,start))
        o['len']=p-start
        objs[oid]=o; order.append(oid)
    return objs,order
if __name__=='__main__':
    import sys,collections
    d=open(sys.argv[1],'rb').read()
    objs,order=parse(d)
    print(len(objs),'objekt'); print(collections.Counter(o['tn'] for o in objs.values()).most_common())
