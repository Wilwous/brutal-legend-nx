#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original geometric NX/PROBE icon. No game artwork, external fonts or assets."""
from PIL import Image,ImageDraw,__version__
GLYPHS={
 'N':['10001','11001','10101','10011','10001','10001','10001'],
 'X':['10001','10001','01010','00100','01010','10001','10001'],
 'P':['11110','10001','10001','11110','10000','10000','10000'],
 'R':['11110','10001','10001','11110','10100','10010','10001'],
 'O':['01110','10001','10001','10001','10001','10001','01110'],
 'B':['11110','10001','10001','11110','10001','10001','11110'],
 'E':['11111','10000','10000','11110','10000','10000','11111']}
def create(path):
    if __version__!='12.3.0':raise RuntimeError('use the pinned Pillow 12.3.0 encoder')
    image=Image.new('RGB',(256,256),(15,19,25));draw=ImageDraw.Draw(image)
    draw.rectangle((12,12,243,243),outline=(255,158,47),width=4)
    def text(word,y,scale,color):
        x=(256-(len(word)*6-1)*scale)//2
        for ch in word:
            for row,pattern in enumerate(GLYPHS[ch]):
                for col,pixel in enumerate(pattern):
                    if pixel=='1':draw.rectangle((x+col*scale,y+row*scale,x+(col+1)*scale-1,y+(row+1)*scale-1),fill=color)
            x+=6*scale
    text('NX',43,14,(255,158,47));text('PROBE',175,6,(235,242,251))
    image.save(path,format='JPEG',quality=92,subsampling=0,optimize=False,progressive=False)
