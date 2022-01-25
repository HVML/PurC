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

def read_cfgs_fin(fin):
    cfgs = []
    line_no = 1
    line = fin.readline()
    while line:
        s = line.strip()
        if s == "" or s[0] == '#':
            line_no = line_no + 1
            line = fin.readline()
            continue

        cfg = s.split()
        cfgs.append({'prefix':cfg[0], 'kw':cfg[1]})
        line = fin.readline()

    return cfgs

def read_cfgs(fn):
    fin = open(fn, "r")
    cfgs = read_cfgs_fin(fin)
    fin.close()
    return cfgs

def gen_PURC_KEYWORD(cfg):
    # generate enums: PCHVML_KEYWORD_<PREFIX>_<KEYWORD>
    prefix = cfg['prefix'].upper()
    kw = cfg['kw'].upper()
    return "    PCHVML_KEYWORD_%s_%s" % (prefix, kw)

def gen_pchvml_keyword(cfg):
    #generate cfgs: { ATOM_BUCKET_<PREFIX>, "<KEYWORD>", 0 }
    prefix = cfg['prefix'].upper()
    kw = cfg['kw']
    return "    { ATOM_BUCKET_%s, \"%s\", 0 }" % (prefix, kw)

def process_header_fn(fout, fin, cfgs):
    line_no = 1
    line = fin.readline()
    while line:
        s = line.strip()
        if s == "%%keywords%%":
            for cfg in cfgs:
                s = gen_PURC_KEYWORD(cfg)
                fout.write("%s,\n" % s)
        else:
            fout.write(line)
        line_no = line_no + 1
        line = fin.readline()

def process_header(dst, src, cfgs):
    fout = open(dst, "w")
    fin = open(src, "r")
    process_header_fn(fout, fin, cfgs)
    fout.close()
    fin.close()

def process_source_fn(fout, fin, cfgs):
    line_no = 1
    line = fin.readline()
    while line:
        s = line.strip()
        if s == "%%keywords%%":
            for cfg in cfgs:
                s = gen_pchvml_keyword(cfg)
                fout.write("%s,\n" % s)
        else:
            fout.write(line)
        line_no = line_no + 1
        line = fin.readline()

def process_source(dst, src, cfgs):
    fout = open(dst, "w")
    fin = open(src, "r")
    process_source_fn(fout, fin, cfgs)
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

    cfgs = read_cfgs(args.kw_txt)
    process_header(args.kw_h, args.kw_h_in, cfgs)
    process_source(args.kw_inc, args.kw_inc_in, cfgs)

