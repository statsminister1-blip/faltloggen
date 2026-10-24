#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# TTF -> BDF (1 bit) med PIL, för U8g2:s bdfconv.  ttf2bdf.py font.ttf px tröskel ut.bdf
import sys
from PIL import Image, ImageDraw, ImageFont

ttf, px, thr, out = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
f = ImageFont.truetype(ttf, px)
asc, desc = f.getmetrics()
codes = list(range(32, 127)) + list(range(160, 256)) + [0x2013, 0x207A, 0x2192, 0x00B7]
glyphs = []
for c in codes:
    ch = chr(c)
    W, H = px * 3, px * 3
    base = px * 2
    im = Image.new('L', (W, H), 0)
    ImageDraw.Draw(im).text((px // 2, base), ch, font=f, fill=255, anchor='ls')
    adv = int(round(f.getlength(ch)))
    bb = im.point(lambda v: 255 if v >= thr else 0).getbbox()
    if bb is None:
        glyphs.append((c, adv, 0, 0, 0, 0, []))
        continue
    x0, y0, x1, y1 = bb
    w, h = x1 - x0, y1 - y0
    rows = []
    for y in range(y0, y1):
        bits = ''.join('1' if im.getpixel((x, y)) >= thr else '0' for x in range(x0, x1))
        bits += '0' * ((8 - len(bits) % 8) % 8)
        rows.append('%0*X' % (len(bits) // 4, int(bits, 2)))
    glyphs.append((c, adv, w, h, x0 - px // 2, base - y1, rows))
with open(out, 'w') as o:
    o.write('STARTFONT 2.1\nFONT -dejavu-%d\nSIZE %d 75 75\n' % (px, px))
    o.write('FONTBOUNDINGBOX %d %d 0 %d\n' % (px * 2, asc + desc, -desc))
    o.write('STARTPROPERTIES 2\nFONT_ASCENT %d\nFONT_DESCENT %d\nENDPROPERTIES\n' % (asc, desc))
    o.write('CHARS %d\n' % len(glyphs))
    for c, adv, w, h, xo, yo, rows in glyphs:
        o.write('STARTCHAR U%04X\nENCODING %d\nSWIDTH 500 0\nDWIDTH %d 0\nBBX %d %d %d %d\nBITMAP\n' % (c, c, adv, w, h, xo, yo))
        for r in rows: o.write(r + '\n')
        o.write('ENDCHAR\n')
    o.write('ENDFONT\n')
