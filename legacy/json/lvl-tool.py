# Convert a .lvl file into a binary representation (.mbl)

# LVL textfile Format:
#
# TITLE
# HINT
# Number of Infotrons to use
# Graphics Set To Use
# Hours
# Minutes
# Seconds
# Width
# Height
#
# >> LEVEL DATA <<
# Level data is Width*Height entries, in row-major order, of the form
#     IconX (assuming the old graphics grids)
#     IconY
#     Trigger ID
#     Trigger IconX
#     Trigger IconY
#     State
#     Kind
#     Level ID (for overhead world map)
#


# MBL Format:
#
# Level Name\0
# Number Of Infotrons as 16-bit unsigned integer
# Width, Height as 16-bit unsigned integers
# Width * Height board entires. Each entry is two unsigned bytes: icon X and icon Y.


# Potential JSON Format in the future?
#
# {
#    version: 0,
#    name: "NAME",
#    hint: "HINT",
#    width: width,
#    height: height,
#    graphics:
#    {
#       old-graphics-set: "Infotron"
#    }
#    board: {
#      [
#         [... Y = 0 ...],
#         [... Y = 1 ...],
#         [ ... { old-icon-x: x, old-icon-y: y, ... } ... ],
#      ]
#    } 
# }

import sys
import os
import struct


lvl_file_name = sys.argv[1]
lvl_file = open(lvl_file_name)
lvl = [line.strip() for line in lvl_file.readlines()]
lvl_file.close()

# Constants
ICON_X = 'icon_x'
ICON_Y = 'icon_y'
TRIGGER_ID = 'trigger_id'
TRIGGER_ICON_X = 'trigger_icon_x'
TRIGGER_ICON_Y = 'trigger_icon_y'
STATE = 'state'
KIND = 'kind'
LEVEL_ID = 'level_id'

print "Reading %s" % lvl_file_name

# Crack the level header
lvl_name = lvl[0]
lvl_hint = lvl[1]
lvl_infotrons = int(lvl[2])
lvl_graphics_set = lvl[3]
lvl_hours = int(lvl[4])
lvl_minutes = int(lvl[5])
lvl_seconds = int(lvl[6])
lvl_width = int(lvl[7])
lvl_height = int(lvl[8])

# Crack the level board data
board = []    # board[y][x] gets us the desired square
current = 9
for y in range(lvl_height):
    board_row = []
    for x in range(lvl_width):
        cell = {}
        cell[ICON_X] = int(lvl[current])
        cell[ICON_Y] = int(lvl[current+1])
        cell[TRIGGER_ID] = int(lvl[current+2])
        cell[TRIGGER_ICON_X] = int(lvl[current+3])
        cell[TRIGGER_ICON_Y] = int(lvl[current+4])
        cell[STATE] = int(lvl[current+5])
        cell[KIND] = int(lvl[current+6])
        cell[LEVEL_ID] = int(lvl[current+7])
        current += 8
        board_row.append(cell)
    board.append(board_row)

# Build the binary file    
mbl_file_name = lvl_file_name[:-4] + ".mbl"
print "Writing %s" % mbl_file_name

mbl_file = open(mbl_file_name, 'wb')

# Write the header
mbl_file.write(struct.pack(">%dsHHH" % (len(lvl_name) + 1), lvl_name, lvl_infotrons, lvl_width, lvl_height))
for y in range(lvl_height):
    for x in range(lvl_width):
        cell = board[y][x]
        mbl_file.write(struct.pack(">BB", cell[ICON_X], cell[ICON_Y]))
        
mbl_file.close()


print "Done."
        




