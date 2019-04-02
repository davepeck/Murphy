# python unbundle_images tile_file tile_names_file target_directory

import os
import os.path
import sys
import json
from PIL import Image

TILES_ACROSS = 10
TILE_WIDTH = 32
TILE_HEIGHT = 32

tile_file_name = sys.argv[1]
tile_names_file_name = sys.argv[2]
target_directory_name = sys.argv[3]

tile_names = json.load(open(tile_names_file_name))
tiles_image = Image.open(tile_file_name)

tile_x = 0
tile_y = 0

for tile_name in tile_names:
    tile_pixel_left = TILE_WIDTH * tile_x
    tile_pixel_top = TILE_HEIGHT * tile_y
    tile_pixel_right = tile_pixel_left + TILE_WIDTH
    tile_pixel_bottom = tile_pixel_top + TILE_HEIGHT
    crop_box = (tile_pixel_left, tile_pixel_top, tile_pixel_right, tile_pixel_bottom)

    single_tile_image = tiles_image.copy()
    single_tile_image = single_tile_image.crop(box=crop_box)

    target_file_name = "{}.png".format(os.path.realpath(os.path.join(target_directory_name, tile_name)))
    print "Saving {}".format(target_file_name)

    single_tile_image.save(target_file_name)

    tile_x += 1
    if tile_x == TILES_ACROSS:
        tile_x = 0
        tile_y += 1
