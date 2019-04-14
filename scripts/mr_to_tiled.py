#!/usr/bin/env python3
# Converts an intermediate MR .json file into a .tilemap.json file suitable
# for loading into the Tiled map editor app.
import base64
import json
import os
import struct
import zlib

import click


def convert(mr_file, tiled_file, is_world):
    mr = json.load(mr_file)
    tile_size = 24 if is_world else 32
    tileset_size = 21 if is_world else 16
    flat_board = [item for sublist in mr["board"] for item in sublist]
    # Add 1 so that firstgid can be 1; tiled appears to treat gid=0 as "no tile here".
    raw_board = [1 + item[0] + item[1] * tileset_size for item in flat_board]
    byte_board = [struct.pack("<l", item) for item in raw_board]
    board_bytes = b"".join(byte_board)
    data = zlib.compress(board_bytes)
    b64data = base64.b64encode(data).decode("ascii")
    tileset_source = (
        "world.tileset.json" if is_world else f"{mr['graphicsSet']}.tileset.json"
    )
    layer = {
        "compression": "zlib",
        "data": b64data,
        "encoding": "base64",
        "height": mr["size"][1],
        "id": 1,
        "name": "level",
        "opacity": 1,
        "type": "tilelayer",
        "visible": True,
        "width": mr["size"][0],
        "x": 0,
        "y": 0,
    }
    tiled = {
        "height": mr["size"][1],
        "infinite": False,
        "layers": [layer],
        "nextlayerid": 2,
        "nextobjectid": 1,
        "orientation": "orthogonal",
        "properties": [
            {"name": "name", "type": "string", "value": mr["name"]},
            {"name": "infotrons", "type": "int", "value": mr["infotrons"]},
        ],
        "renderorder": "right-down",
        "tiledversion": "1.2.3",
        "tileheight": tile_size,
        "tilesets": [{"firstgid": 1, "source": tileset_source}],
        "tilewidth": tile_size,
        "type": "map",
        "version": "1.2",
        "width": mr["size"][0],
    }
    json.dump(tiled, tiled_file, indent=2)


@click.command()
@click.argument("src", type=click.Path(exists=True, resolve_path=True), nargs=-1)
@click.argument("dst", type=click.Path(exists=True, resolve_path=True), nargs=1)
def mlv_to_json(src, dst):
    for mr_file_name in src:
        mr_basename = os.path.basename(mr_file_name)
        mr_barename = os.path.splitext(mr_basename)[0]
        tiled_file_name = os.path.join(dst, f"{mr_barename}.tilemap.json")
        with open(mr_file_name, "rt") as mr_file, open(
            tiled_file_name, "wt"
        ) as tiled_file:
            print(f"Converting {mr_file_name} to {tiled_file_name}")
            convert(mr_file, tiled_file, is_world="World.json" in mr_file_name)


if __name__ == "__main__":
    mlv_to_json()
