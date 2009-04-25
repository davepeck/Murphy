#!/usr/bin/env python2.5

# Turn a bunch of old-school tiles bitmap files into a FUCKING AWESOME
# image suitable for use with opengl. Importantly, keep the ordering
# of the tiles the same as what we did in the original murphy. Which
# I admit is kind of a nuts thing to do, but then our levels are nice.

import os
import sys
from PIL import Image

TILE_SIZE = 32
TEXTURE_TILES_ACROSS = 10

# (file name, vertical separation, total-size)
# if a file is not found, or does not meet its total-size, it is padded (or cropped) automagically

FILE_INFO = [
    ("basic-pieces.bmp", 0, 10),    # 10
    ("ports.bmp", 0, 7),            # 17 
    ("zonks-infotron.bmp", 0, 3),   # 20
    ("basic-pieces-2.bmp", 0, 60),  # 80
    ("laser.bmp", 1, 14),           # 94
    ("infotron.bmp", 1, 16),        # 110
    ("terminal.bmp", 1, 12),        # 122
    ("bug.bmp", 0, 6),              # 128
    ("explosion.bmp", 0, 10),       # 138
    ("exit.bmp", 0, 8),
    ("zaps.bmp", 1, 33),
    ("red-disk.bmp", 0, 2),
    ("red-zonk.bmp", 0, 4),
    ("blue-zonk.bmp", 0, 4),
    ("plasmoid.bmp", 0, 9),
    ("robot.bmp", 1, 16),
    ("oliver.bmp", 1, 90) ]
    
def fill_rect(target_pixels, rect, color):
    l, t, w, h = rect
    for x in range(l, l+w):
        for y in range(t, t+h):
            target_pixels[x, y] = color

def copy_rect(source_pixels, target_pixels, source_rect, target_rect):
    sl, st, sw, sh = source_rect
    tl, tt, tw, th = target_rect

    sx = sl
    for tx in range(tl, tl+tw):
        sy = st
        for ty in range(tt, tt+th):
            target_pixels[tx, ty] = source_pixels[sx, sy]
            sy += 1
        sx += 1

def fill_tile(target_pixels, tile_x, tile_y, color):
    fill_rect(target_pixels, (tile_x * TILE_SIZE, tile_y * TILE_SIZE, TILE_SIZE, TILE_SIZE), color)

def copy_tile(source_pixels, target_pixels, source_tile_x, source_tile_y, source_y_separation, target_tile_x, target_tile_y):
    source_rect = (source_tile_x * TILE_SIZE, source_tile_y * (TILE_SIZE + source_y_separation), TILE_SIZE, TILE_SIZE)
    target_rect = (target_tile_x * TILE_SIZE, target_tile_y * TILE_SIZE, TILE_SIZE, TILE_SIZE)
    copy_rect(source_pixels, target_pixels, source_rect, target_rect)
    
if __name__ == "__main__":
    texture = Image.new("RGBA", (512, 1024), (255, 0, 0, 255))
    texture_pixels = texture.load()

    texture_tile_x = 0
    texture_tile_y = 0
    
    for file_name, y_separation, tile_count in FILE_INFO:
        print "merging %s" % file_name
        
        try:
            tiles = Image.open(file_name)
            tile_pixels = tiles.load()
        except:
            tiles = None
            tile_pixels = None

        source_tile_x = 0
        source_tile_y = 0        

        if tiles is None:
            file_tiles_across = 0
            file_tiles_down = 0
        else:
            file_pixel_width, file_pixel_height = tiles.size
            file_tiles_across = file_pixel_width / TILE_SIZE
            file_tiles_down = (file_pixel_height + y_separation) / (TILE_SIZE + y_separation)
        
        for tile_number in range(0, tile_count):
            if source_tile_x >= file_tiles_across or source_tile_y >= file_tiles_down:
                fill_tile(texture_pixels, texture_tile_x, texture_tile_y, (0, 0, 255))
            else:
                copy_tile(tile_pixels, texture_pixels, source_tile_x, source_tile_y, y_separation, texture_tile_x, texture_tile_y)
                
            texture_tile_x += 1
            if texture_tile_x >= TEXTURE_TILES_ACROSS:
                texture_tile_x = 0
                texture_tile_y += 1

            source_tile_x += 1
            if source_tile_x >= file_tiles_across:
                source_tile_x = 0
                source_tile_y += 1

    texture.save("texture.png", "png")
            

