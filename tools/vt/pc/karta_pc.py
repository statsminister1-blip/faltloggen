# SPDX-License-Identifier: GPL-3.0-or-later
# Gör karta_pc.h av ../../../src/Faltloggen/karta.h utan nätdelen (HTTP, JPEG), för kartatest.cpp och falttest.cpp
import sys, os
src = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../../src/Faltloggen/karta.h')).read()
for inc in ['#include <HTTPClient.h>\n', '#include <WiFiClientSecure.h>\n', '#include "rom/tjpgd.h"\n']:
    src = src.replace(inc, '')
a = src.index('// JPEG -> RGB565'); b = src.index('// Ett steg: hoppa över rutor som finns')
src = src[:a] + ('static uint8_t *kDlBuf = nullptr;\nstatic uint16_t *kDlPx = nullptr;\n'
                 'static void kTilePath(char *p, int n, int z, int x, int y) { snprintf(p, n, "%s/%d/%d_%d.RGB", K_KALLOR[kKalla].mapp, z, x, y); }\n'
                 'static int kDownload(int, int, int) { return -1; }\n\n') + src[b:]
open(sys.argv[1] if len(sys.argv) > 1 else 'karta_pc.h', 'w').write(src)
