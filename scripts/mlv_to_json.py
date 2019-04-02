# Converts an ancient .mvl file into a still-not-perfect .json file
import json
import os
import struct

import click


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


class MurphyLevel:
    @classmethod
    def from_mlv_file(cls, mlv_file):
        scanner = BinaryDataScanner(mlv_file)
        level_name = scanner.read_ascii_string()
        graphics_set = scanner.read_ascii_string()
        infotrons = scanner.read_16()
        width = scanner.read_16()
        height = scanner.read_16()

        board = []

        for y in range(height):
            row = []
            for x in range(width):
                point = scanner.read_byte(), scanner.read_byte()
                row.append(point)
            board.append(row)

        return cls(level_name, graphics_set, infotrons, width, height, board)

    def __init__(self, level_name, graphics_set, infotrons, width, height, board):
        self.level_name = level_name
        self.graphics_set = graphics_set
        self.infotrons = infotrons
        self.width = width
        self.height = height
        self.board = board

    def to_jsonable(self):
        return {
            "name": self.level_name,
            "graphicsSet": self.graphics_set,
            "infotrons": self.infotrons,
            "size": [self.width, self.height],
            "board": self.board,
        }


def convert_mlv(mlv_file, json_file):
    level = MurphyLevel.from_mlv_file(mlv_file)
    jsonable = level.to_jsonable()
    json.dump(jsonable, json_file)


@click.command()
@click.argument("src", type=click.Path(exists=True, resolve_path=True), nargs=-1)
@click.argument("dst", type=click.Path(exists=True, resolve_path=True), nargs=1)
def mlv_to_json(src, dst):
    for mlv_file_name in src:
        mlv_basename = os.path.basename(mlv_file_name)
        mlv_barename = os.path.splitext(mlv_basename)[0]
        json_file_name = os.path.join(dst, f"{mlv_barename}.json")
        with open(mlv_file_name, "rb") as mlv_file, open(
            json_file_name, "wt"
        ) as json_file:
            print(f"Converting {mlv_file_name} to {json_file_name}")
            convert_mlv(mlv_file, json_file)


if __name__ == "__main__":
    mlv_to_json()
