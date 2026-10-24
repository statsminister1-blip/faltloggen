# SPDX-License-Identifier: GPL-3.0-or-later
# Gör om Fältloggens logga (bilder/logo.png) till en 4-bitars alfamask i en C-header (src/Faltloggen/logo_data.h).
# Mörkt = synligt (alfa 15), vitt = bakgrund. Skärmen färgar masken: mörk på ljust i dagläge, ljus på mörkt i nattläge.
#   python3 tools/genlogo.py bilder/logo.png src/Faltloggen/logo_data.h
import sys
from PIL import Image
import numpy as np
src, out = sys.argv[1], sys.argv[2]
g = np.array(Image.open(src).convert('L')).astype(np.float32)
a = np.clip((250 - g) / 235.0, 0, 1)
rows = np.where(a.max(1) > 0.1)[0]; cols = np.where(a.max(0) > 0.1)[0]
a = a[rows.min():rows.max() + 1, cols.min():cols.max() + 1]
lines = ['// Genererad av tools/genlogo.py ur bilder/logo.png - 4 bitar alfa per pixel, två pixlar per byte',
         '#pragma once', '#include <stdint.h>', '']
for name, h in (('LOGO_BIG', 330),):
    h0, w0 = a.shape
    w = max(1, round(w0 * h / h0))
    im = Image.fromarray((a * 255).astype(np.uint8)).resize((w, h), Image.LANCZOS)
    q = np.clip(np.round(np.array(im) / 17.0), 0, 15).astype(np.uint8)
    wp = (w + 1) // 2 * 2
    qp = np.zeros((h, wp), np.uint8); qp[:, :w] = q
    data = ((qp[:, 0::2] << 4) | qp[:, 1::2]).flatten()
    lines += [f'#define {name}_W {w}', f'#define {name}_H {h}', f'static const uint8_t {name}[{len(data)}] = {{']
    for i in range(0, len(data), 24):
        lines.append('  ' + ','.join(f'0x{b:02X}' for b in data[i:i + 24]) + ',')
    lines += ['};', '']
    print(name, w, h, len(data))
open(out, 'w').write('\n'.join(lines))
