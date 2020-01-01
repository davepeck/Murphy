#!/usr/bin/env python3
#
# Takes a tiled tilemap.json that has (1) compressed layer data, and (2)
# linked reference to a tileset and (a) uncompresses the layer data, as well
# as (b) directly embeds the tileset data. *Sigh*. -Dave

import base64
import json
import os
import struct
import zlib

import click


def uncompress(source_file, target_file):
    source = json.load(source_file)

    # Uncompress the layer
    layer = source["layers"][0]
    del layer["encoding"]
    del layer["compression"]
    data = base64.b64decode(layer["data"])
    board_bytes = zlib.decompress(data)
    r = range(0, len(board_bytes), 4)
    byte_board = [struct.unpack("<l", board_bytes[i : i + 4])[0] for i in r]
    layer["data"] = byte_board

    # Fix the tilesets by opening the referenced file and w
    tileset = source["tilesets"][0]
    tileset_source_file_name = tileset["source"]
    tileset_source_file_name = os.path.join(
        os.path.dirname(source_file.name), f"./{tileset_source_file_name}"
    )
    del tileset["source"]
    with open(tileset_source_file_name, "rt") as tileset_source_file:
        tileset_source = json.load(tileset_source_file)
    tileset.update(tileset_source)

    json.dump(source, target_file, indent=2)


@click.command()
@click.argument("src", type=click.Path(exists=True, resolve_path=True), nargs=-1)
@click.argument("dst", type=click.Path(exists=True, resolve_path=True), nargs=1)
def uncompress_tiled(src, dst):
    for source_file_name in src:
        source_base_name = os.path.basename(source_file_name)
        source_base_name = os.path.splitext(source_base_name)[0]
        target_file_name = os.path.join(dst, f"{source_base_name}.json")
        with open(source_file_name, "rt") as source_file, open(
            target_file_name, "wt"
        ) as target_file:
            print(f"Uncompressing {source_file_name} to {target_file_name}")
            uncompress(source_file, target_file)


if __name__ == "__main__":
    uncompress_tiled()
