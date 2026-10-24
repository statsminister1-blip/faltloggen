# SPDX-License-Identifier: GPL-3.0-or-later
import numpy as np, sys
from PIL import Image
a=np.fromfile(sys.argv[1],dtype='<u2').reshape(480,800)
r=((a>>11)&31)*255//31; g=((a>>5)&63)*255//63; b=(a&31)*255//31
Image.fromarray(np.dstack([r,g,b]).astype('uint8')).save(sys.argv[2])
