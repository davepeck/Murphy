#!/usr/bin/env python2.5

import os
import sys


x = os.popen('ls *.lvl')
y = [line.strip() for line in x.readlines()]
x.close()

#x = ""
for lvl in y:
    #x += '@"%s", ' % lvl.split(".")[0]
    x = os.popen('./assemble-binary-level.py "%s"' % lvl)
    print x.read()
    x.close()

#print x


