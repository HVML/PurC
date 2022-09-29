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
    cfgs = {}
    line_no = 1
    line = fin.readline()
    while line:
        s = line.strip()
        if s == "" or s[0] == '#':
            line_no = line_no + 1
            line = fin.readline()
            continue

        cfg = s.split()

        prefix = cfg[0]

        for i in range(len(cfg)):
            if i > 0:
                if not prefix in cfgs:
                    cfgs[prefix] = []

                cfgs[prefix].append(cfg[i])

        line = fin.readline()

    return cfgs

def read_cfgs(fn):
    fin = open(fn, "r")
    cfgs = read_cfgs_fin(fin)
    for cfg in cfgs:
        cfgs[cfg] = sorted(cfgs[cfg])
    fin.close()
    return cfgs

def gen_PURC_KEYWORD(idx, nr, first, prefix, kw):
    # generate enums:    PCHVML_KEYWORD_<PREFIX>_<KEYWORD>
    if idx == first:
        s = "/*=*/"
    else:
        s = "     "
    return "    /* %*d */ %s PCHVML_KEYWORD_%s_%s" % (len(str(nr)), idx, s, prefix.upper(), kw.upper().replace('-', '_'))

def gen_pchvml_keyword(idx, nr, first, prefix, sz, kw):
    # generate cfgs:     { 0, "<KEYWORD>" }
    if idx == first:
        s = "/*=*/"
    else:
        s = "     "
    return "    /* ATOM_BUCKET_%-*s */ %s { 0, \"%s\" }" % (len(str(sz)), prefix.upper(), s, kw)

def gen_keywords_bucket_init(prefix, start, end):
    # generate func_calls:
    #   keywords_bucket_init(keywords, start, end, ATOM_BUCKET_<PREFIX>)
    return "    keywords_bucket_init(keywords, %s, %s, ATOM_BUCKET_%s)" % (start, end, prefix.upper())

def process_header_fn(fout, fin, cfgs):
    line_no = 1
    line = fin.readline()
    while line:
        s = line.strip()
        if s == "%%keywords%%":
            idx = 0
            nr = 0
            for prefix in cfgs:
                kws = cfgs[prefix]
                nr += len(kws)

            for prefix in cfgs:
                kws = cfgs[prefix]
                first = idx
                for kw in kws:
                    s = gen_PURC_KEYWORD(idx, nr, first, prefix, kw)
                    fout.write("%s,\n" % s)
                    idx += 1
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
            sz = 0;
            for prefix in cfgs:
                if sz < len(prefix):
                    sz = len(prefix)

            idx = 0
            nr = 0
            for prefix in cfgs:
                kws = cfgs[prefix]
                nr += len(kws)

            for prefix in cfgs:
                kws = cfgs[prefix]
                first = idx
                for kw in kws:
                    s = gen_pchvml_keyword(idx, nr, first, prefix, sz, kw)
                    fout.write("%s,\n" % s)
                    idx += 1
        elif s == "%%keywords_bucket_init%%":
            start = 0
            for prefix in cfgs:
                kws = cfgs[prefix]
                nr = len(kws)
                end = start + nr;
                s = gen_keywords_bucket_init(prefix, start, end)
                fout.write("%s;\n" % s)
                start = end
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

