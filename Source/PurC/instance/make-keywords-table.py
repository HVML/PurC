#!/usr/bin/python3

#
# Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
#
# This file is a part of Purring Cat 2, a HVML parser and interpreter.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# Author: Vincent Wei <https://github.com/VincentWei>

"""
Make HVML keywords table:
    1. Read 'data/keywords.txt' file.
    2. Generate the keywords.h and keywords.inc
"""

import argparse

def read_kws_fin(fin):
    kws = {}
    line_no = 1
    line = fin.readline()
    while line:
        s = line.strip()
        if s == "" or s[0] == '#':
            line_no = line_no + 1
            line = fin.readline()
            continue

        kws[s] = 1
        line = fin.readline()

    return kws

def read_kws(fn):
    fin = open(fn, "r")
    kws = read_kws_fin(fin)
    fin.close()
    return kws

def gen_PURC_KEYWORD(kw):
    return "    PCHVML_KEYWORD_%s" % kw.upper()

def gen_pchvml_keyword(kw):
    return "    { {}, \"%s\", 0 }" % kw

def process_header_fn(fout, fin, kws):
    line_no = 1
    line = fin.readline()
    while line:
        s = line.strip()
        if s == "%%keywords%%":
            for kw in kws:
                s = gen_PURC_KEYWORD(kw)
                fout.write("%s,\n" % s)
        else:
            fout.write(line)
        line_no = line_no + 1
        line = fin.readline()

def process_header(dst, src, kws):
    fout = open(dst, "w")
    fin = open(src, "r")
    process_header_fn(fout, fin, kws)
    fout.close()
    fin.close()

def process_source_fn(fout, fin, kws):
    line_no = 1
    line = fin.readline()
    while line:
        s = line.strip()
        if s == "%%keywords%%":
            for kw in kws:
                s = gen_pchvml_keyword(kw)
                fout.write("%s,\n" % s)
        else:
            fout.write(line)
        line_no = line_no + 1
        line = fin.readline()

def process_source(dst, src, kws):
    fout = open(dst, "w")
    fin = open(src, "r")
    process_source_fn(fout, fin, kws)
    fout.close()
    fin.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generating keywords table')
    parser.add_argument('--dest')
    parser.add_argument('--kw_h')
    parser.add_argument('--kw_inc')
    parser.add_argument('--kw_foo')
    parser.add_argument('--kw_txt')
    parser.add_argument('--kw_h_in')
    parser.add_argument('--kw_inc_in')
    parser.add_argument('--without-print', action='store_true')
    args = parser.parse_args()
    # parser.print_help()
    # print(args)

    kws = read_kws(args.kw_txt)
    process_header(args.kw_h, args.kw_h_in, kws)
    process_source(args.kw_inc, args.kw_inc_in, kws)

