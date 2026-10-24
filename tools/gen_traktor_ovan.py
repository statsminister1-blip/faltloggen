# SPDX-License-Identifier: GPL-3.0-or-later
# Ritar kartans traktormarkör (traktorn ovanifrån, fronten åt vänster) och skriver src/Faltloggen/traktor_ovan.h.
# En enkel, egen ritning (inget foto): svarta hjul, grå motorhuv, ljus hytt. Antennen (GPS) antas vid x = 37.
#   python3 tools/gen_traktor_ovan.py src/Faltloggen/traktor_ovan.h
import sys
from PIL import Image, ImageDraw
W, H, S = 64, 40, 8
im = Image.new('RGB', (W * S, H * S), (255, 0, 255))
d = ImageDraw.Draw(im)
def R(x0, y0, x1, y1, c, r=0):
    d.rounded_rectangle([x0 * S, y0 * S, x1 * S - 1, y1 * S - 1], radius=r * S, fill=c)
svart, gra, morkgra, ljus, kant = (25, 25, 25), (120, 124, 128), (70, 72, 75), (225, 230, 232), (35, 35, 35)
R(5, 1, 19, 10, svart, 2); R(5, 30, 19, 39, svart, 2)          # framhjul
R(40, 0, 61, 11, svart, 2); R(40, 29, 61, 40, svart, 2)        # bakhjul
R(2, 13, 31, 27, kant, 4); R(3, 14, 30, 26, gra, 3)            # motorhuv
R(4, 18, 9, 22, morkgra, 1)                                    # grill / framvikt
R(29, 9, 58, 31, kant, 3); R(30, 10, 57, 30, ljus, 2)           # hytt (tak)
R(33, 13, 54, 27, (200, 206, 210), 2)
R(58, 15, 63, 25, morkgra, 1)                                  # dragkrok / lyft
im = im.resize((W, H), Image.LANCZOS)
px = im.load()
out = []
for y in range(H):
    for x in range(W):
        r, g, b = px[x, y]
        if r > 200 and b > 200 and g < 80: v = 0xF81F
        else:
            v = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            if v == 0xF81F: v = 0xF81E
        out.append(v)
L = ['// Traktorn ovanifrån som markör på kartan (egen ritning, tools/gen_traktor_ovan.py). Fronten åt vänster (-x).',
     '// RGB565, 0xF81F = genomskinlig.', '#pragma once', '#define TROVAN_W 64', '#define TROVAN_H 40', '#define TROVAN_KEY 0xF81F',
     'static const uint16_t TROVAN[TROVAN_W * TROVAN_H] = {']
for i in range(0, len(out), 16): L.append('  ' + ','.join('0x%04X' % v for v in out[i:i + 16]) + ',')
L.append('};')
open(sys.argv[1], 'w').write('\n'.join(L) + '\n')
im.resize((W * 6, H * 6), Image.NEAREST).save('/home/claude/pcb/ovan_ny.png')
