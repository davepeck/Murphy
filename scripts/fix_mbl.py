#!/usr/bin/env python3
# Converts an ancient .mbl file into a slightly more modern .mlv file
import sys
import struct


class BinaryDataScanner(object):
    def __init__(self, f):
        self.f = f

    def read_byte(self):
        return struct.unpack(">B", self.f.read(1))[0]

    def read_16(self):
        return struct.unpack(">H", self.f.read(2))[0]

    def read_ascii_string(self):
        bytes = b""
        b = self.f.read(1)
        while ord(b) != 0:
            bytes += b
            b = self.f.read(1)
        return bytes.decode("ascii")


class BinaryDataWriter(object):
    def __init__(self, f):
        self.f = f

    def write_byte(self, byte):
        b = struct.pack(">B", byte)
        self.f.write(b)

    def write_16(self, short):
        b = struct.pack(">H", short)
        self.f.write(b)

    def write_ascii_string(self, s):
        b = s.encode("ascii")
        self.f.write(b)
        self.f.write(b"\x00")


class MurphyLevel(object):
    def __init__(self, name, graphics, infotrons, width, height, grid):
        self.name = name
        self.graphics = graphics
        self.infotrons = infotrons
        self.width = width
        self.height = height
        self.grid = grid

    @classmethod
    def load_from_mbl_file(cls, f, is_world):
        scanner = BinaryDataScanner(f)
        name = scanner.read_ascii_string()
        graphics = scanner.read_ascii_string()
        infotrons = scanner.read_16()
        width = scanner.read_16()
        height = scanner.read_16()

        grid = []

        actual_width = width
        actual_height = height

        # Pass one: full width/height
        for y in range(height):
            for x in range(width):
                tilemap_x = scanner.read_byte()
                tilemap_y = scanner.read_byte()
                grid.append((tilemap_x, tilemap_y))

                if not is_world:
                    # Look for bottom-right border
                    tilemap = (tilemap_y * 10) + tilemap_x
                    if tilemap == 37:
                        actual_width = x + 1
                        actual_height = y + 1

        # Pass two: fixed width/height
        fixed_grid = []
        grid_index = 0
        for y in range(height):
            for x in range(width):
                if (y < actual_height) and (x < actual_width):
                    fixed_grid.append(grid[grid_index])
                grid_index += 1

        return MurphyLevel(
            name, graphics, infotrons, actual_width, actual_height, fixed_grid
        )

    def save_to_mlv_file(self, f):
        writer = BinaryDataWriter(f)
        writer.write_ascii_string(self.name)
        writer.write_ascii_string(self.graphics)
        writer.write_16(self.infotrons)
        writer.write_16(self.width)
        writer.write_16(self.height)

        grid_index = 0
        for y in range(self.height):
            for x in range(self.width):
                writer.write_byte(self.grid[grid_index][0])
                writer.write_byte(self.grid[grid_index][1])
                grid_index += 1


if __name__ == "__main__":
    input_name = sys.argv[1]
    output_name = ".".join(input_name.split(".")[:-1]) + ".mlv"

    print("Converting from {} to {}...".format(input_name, output_name))

    with open(input_name, "rb") as input_file:
        level = MurphyLevel.load_from_mbl_file(
            input_file, is_world="World.mbl" in input_name
        )
        with open(output_name, "wb") as output_file:
            level.save_to_mlv_file(output_file)

    print("\t...done")
