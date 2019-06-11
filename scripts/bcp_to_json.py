#!/usr/bin/env python3
# Converts an ancient .bcp binary, which is the original compressed
# data structure used in the Infotron for System 7's resource fork,
# into a still-not-perfect .json file
import json
import os
import struct

import click


class BinaryDataScanner:
    UINT_8 = ">B"
    INT_8 = ">b"
    UINT_16 = ">H"
    INT_16 = ">h"
    UINT_32 = ">I"
    INT_32 = ">i"

    def __init__(self, f):
        self.f = f

    def read_byte(self):
        return struct.unpack(self.UINT_8, self.f.read(1))[0]

    def iter_read_byte(self):
        for unpacked in struct.iter_unpack(self.UINT_8, self.f.read()):
            yield unpacked[0]

    def read_uchar(self):
        return struct.unpack(self.INT_8, self.f.read(1))[0]

    def iter_read_uchar(self):
        for unpacked in struct.iter_unpack(self.INT_8, self.f.read()):
            yield unpacked[0]

    def read_uint_16(self):
        return struct.unpack(self.UINT_16, self.f.read(2))[0]

    def iter_read_uint_16(self):
        for unpacked in struct.iter_unpack(self.UINT_16, self.f.read()):
            yield unpacked[0]

    def read_int_16(self):
        return struct.unpack(self.INT_16, self.f.read(2))[0]

    def iter_read_int_16(self):
        for unpacked in struct.iter_unpack(self.INT_16, self.f.read()):
            yield unpacked[0]

    def read_int_32(self):
        return struct.unpack(self.INT_32, self.f.read(4))[0]

    def iter_read_int_32(self):
        for unpacked in struct.iter_unpack(self.INT_32, self.f.read()):
            yield unpacked[0]

    def read_c_string(self, encoding="ascii"):
        """Read a null-terminated C string."""
        bytes = b""
        b = self.f.read(1)
        while ord(b) != 0:
            bytes += b
            b = self.f.read(1)
        return bytes.decode(encoding)

    def read_pascal_string(self, encoding="mac-roman"):
        """Read a pascal str255 with length byte up front."""
        block = self.f.read(256)
        length = block[0]
        bytes = block[1:][:length]
        return bytes.decode(encoding)


class CompressedBoard:
    @classmethod
    def from_bcp_file(cls, bcp_file):
        scanner = BinaryDataScanner(bcp_file)
        name = scanner.read_pascal_string()
        size_x = scanner.read_int_16()
        size_y = scanner.read_int_16()
        murphy = (scanner.read_byte() - 1, scanner.read_byte() - 1)
        term = (scanner.read_byte() - 1, scanner.read_byte() - 1)
        num_scissors = scanner.read_int_16()
        num_quarks = scanner.read_int_16()
        num_infotrons = scanner.read_int_16()
        num_bugs = scanner.read_int_16()
        num_compressions = scanner.read_int_16()
        compressed_board = list(scanner.iter_read_int_16())
        return cls(
            name,
            size_x,
            size_y,
            murphy,
            term,
            num_scissors,
            num_quarks,
            num_infotrons,
            num_bugs,
            num_compressions,
            compressed_board,
        )

    def __init__(
        self,
        name,
        size_x,
        size_y,
        murphy,
        term,
        num_scissors,
        num_quarks,
        num_infotrons,
        num_bugs,
        num_compressions,
        compressed_board,
    ):
        self.name = name
        self.size_x = size_x
        self.size_y = size_y
        self.murphy = murphy
        self.term = term
        self.num_scissors = num_scissors
        self.num_quarks = num_quarks
        self.num_infotrons = num_infotrons
        self.num_bugs = num_bugs
        self.num_compressions = num_compressions
        self.compressed_board = compressed_board


class LevelRecord:
    @classmethod
    def from_compressed_board(cls, compressed_board):
        name = compressed_board.name
        size_x = compressed_board.size_x
        size_y = compressed_board.size_y
        murphy = compressed_board.murphy
        term = compressed_board.term
        num_scissors = compressed_board.num_scissors
        num_quarks = compressed_board.num_quarks
        num_infotrons = compressed_board.num_infotrons
        num_bugs = compressed_board.num_bugs

        # Skip a bogus integer that appears to be packed in all levels
        i_cb = 0 + 1

        scissors = [None] * num_scissors
        for i_scissors in range(num_scissors):
            value = compressed_board.compressed_board[i_cb]
            scissors[i_scissors] = ((value // 256) - 1, (value % 256) - 1)
            i_cb += 1

        quarks = [None] * num_quarks
        for i_quarks in range(num_quarks):
            value = compressed_board.compressed_board[i_cb]
            quarks[i_quarks] = ((value // 256) - 1, (value % 256) - 1)
            i_cb += 1

        bugs = [None] * num_bugs
        for i_bugs in range(num_bugs):
            value = compressed_board.compressed_board[i_cb]
            bugs[i_bugs] = ((value // 256) - 1, (value % 256) - 1)
            i_cb += 1

        board = []
        for x in range(size_x):
            board.append([128] * size_y)
        i_x = 0
        i_y = 0

        last_value = 128
        while i_cb < compressed_board.num_compressions:
            value = compressed_board.compressed_board[i_cb]
            i_cb += 1

            if value < 0:
                for q in range(1, -value - 1 + 1):
                    board[i_x][i_y] = last_value
                    i_x += 1
                    if i_x >= size_x:
                        i_y += 1
                        i_x = 0
            else:
                board[i_x][i_y] = value
                last_value = value

                i_x += 1
                if i_x >= size_x:
                    i_y += 1
                    i_x = 0

        # find actual extent of board...
        test_x = size_x - 1
        while board[test_x][1] == 128:
            test_x -= 1
        size_x = test_x + 1

        test_y = size_y - 1
        while board[1][test_y] == 128:
            test_y -= 1
        size_y = test_y + 1

        # ... and crop the board
        cropped_board = [inner[:size_y] for inner in board[:size_x]]

        # ... and fix one other completely nonsense bug in the original
        # DecompressBoard code.
        cropped_board[-1][-1] = 1053  # "botrt"

        return cls(
            name,
            size_x,
            size_y,
            murphy,
            term,
            num_scissors,
            num_quarks,
            num_infotrons,
            num_bugs,
            scissors,
            quarks,
            bugs,
            cropped_board,
        )

    def __init__(
        self,
        name,
        size_x,
        size_y,
        murphy,
        term,
        num_scissors,
        num_quarks,
        num_infotrons,
        num_bugs,
        scissors,
        quarks,
        bugs,
        board,
    ):
        self.name = name
        self.size_x = size_x
        self.size_y = size_y
        self.murphy = murphy
        self.term = term
        self.num_scissors = num_scissors
        self.num_quarks = num_quarks
        self.num_infotrons = num_infotrons
        self.num_bugs = num_bugs
        self.scissors = scissors
        self.quarks = quarks
        self.bugs = bugs
        self.board = board

    def to_jsonable(self):
        return {
            "name": self.name,
            "graphicsSet": "classic",
            "size": [self.size_x, self.size_y],
            "infotrons": self.num_infotrons,
            "murphy": self.murphy,
            "term": self.term,
            "board": self.board,
        }


def convert_bcp(bcp_file, json_file):
    compressed_board = CompressedBoard.from_bcp_file(bcp_file)
    level_record = LevelRecord.from_compressed_board(compressed_board)
    jsonable = level_record.to_jsonable()
    json_string = json.dumps(jsonable, ensure_ascii=False)
    json_bytes = json_string.encode("utf8")
    json_file.write(json_bytes)


@click.command()
@click.argument("src", type=click.Path(exists=True, resolve_path=True), nargs=-1)
@click.argument("dst", type=click.Path(exists=True, resolve_path=True), nargs=1)
def bcp_to_json(src, dst):
    for bcp_file_name in src:
        bcp_basename = os.path.basename(bcp_file_name)
        bcp_barename = os.path.splitext(bcp_basename)[0]
        json_file_name = os.path.join(dst, f"{bcp_barename}.json")
        with open(bcp_file_name, "rb") as bcp_file, open(
            json_file_name, "wb"
        ) as json_file:
            print(f"Converting {bcp_file_name} to {json_file_name}")
            convert_bcp(bcp_file, json_file)


if __name__ == "__main__":
    bcp_to_json()
