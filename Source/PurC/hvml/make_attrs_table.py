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
Make HVML attrs table:
    1. Read 'data/attrs.txt' file.
    5. Generate the hvml_attr_static_list_t table.
    5. Write code to 'hvml_attr_static_list.inc'.
"""

import os, sys
import time
import re

# Find and append run script run dir to module search path
ABS_PATH = os.path.dirname(os.path.abspath(__file__))
sys.path.append("{}/../Scripts/".format(ABS_PATH))

import LXB

WITHOUT_PRINT = 0

TOOL_NAME="make_hvml_attrs_table.py"
SRC_FILE="data/attrs.txt"
INC_FILE="attr_static_list.inc"
H_FILE="attr.h"

RE_START_WITH_TYPE = re.compile(r"^\s+type:")
def start_with_type(line):
    if RE_START_WITH_TYPE.match(line) == None:
        return False
    return True

RE_START_WITH_ALIAS = re.compile(r"^\s+alias:")
def start_with_alias(line):
    if RE_START_WITH_ALIAS.match(line) == None:
        return False
    return True

RE_START_WITH_SPACE = re.compile(r"^\s")
def start_with_space(line):
    if RE_START_WITH_SPACE.match(line) == None:
        return False
    return True

def get_value(line):
    fragments = line.split(":")

    if len(fragments) == 2:
        value = fragments[1].strip()
        if len(value) == 0:
            return None
        return value

    return None

def scan_src_file(f_inc):
    def_info = {}
    attr_info = {}

    attr_token = ""
    value_line = ""
    line_no = 1
    org_line = f_inc.readline()
    while org_line:
        stripped_line = org_line.strip()
        if stripped_line == "" or stripped_line[0] == '#':
            line_no = line_no + 1
            org_line = f_inc.readline()
            continue

        elif start_with_type (org_line):
            type_value = get_value(stripped_line)
            if type_value is None:
                if not WITHOUT_PRINT:
                    print("scan_src_file (Line %d): type value expected (%s)" % (line_no, stripped_line, ))
                return None
            else:
                attr_info[attr_token]['type'] = type_value

        elif start_with_alias (org_line):
            alias_value = get_value(stripped_line)
            if alias_value is None:
                if not WITHOUT_PRINT:
                    print("scan_src_file (Line %d): alias value expected (%s)" % (line_no, stripped_line, ))
                return None
            else:
                attr_info[attr_token]['alias'] = alias_value

        elif not start_with_space (org_line):
            attr_token = stripped_line
            if attr_token in attr_info:
                if not WITHOUT_PRINT:
                    print("scan_src_file (Line %d): duplicated property name (%s)" % (line_no, stripped_line, ))
                return None
            attr_info[attr_token] = {}
        else:
            if not WITHOUT_PRINT:
                print("scan_src_file (Line %d): syntax error %s (%s)" % (line_no, attr_token, stripped_line, ))
            return None

        line_no = line_no + 1
        org_line = f_inc.readline()

    return attr_info

def str2key_simple (_str):
    _bytes = _str.encode()
    _len = len (_str)
    return _bytes[0] * _bytes [_len - 1]

def calc_hash_degree (attr_info, nr_slots):

    degrees = []
    for i in range (0, nr_slots):
        degrees.append (0)

    min_degree = nr_slots
    max_degree = 0

    for attr in attr_info:
        slot_idx = attr_info[attr]['key'] % nr_slots
        degrees [slot_idx] += 1

    nr_zeros = 0
    for d in degrees:
        if d > max_degree:
            max_degree = d
        if d < min_degree:
            min_degree = d
        if d == 0:
            nr_zeros += 1

    if not WITHOUT_PRINT:
        print("%s: min/max hash degree: (%d, %d) / %d; zero slots: %d; %d" % (TOOL_NAME, min_degree, max_degree, nr_slots, nr_zeros, nr_zeros * max_degree))
    return nr_zeros * max_degree

def make_attr_type(attr_token):
    attr_id = attr_token.upper()
    attr_id = attr_id.replace('-', '_')
    attr_id = attr_id.replace('!', '_')

    return "PCHVML_ATTR_TYPE_" + attr_id;

def generate_static_attr_table (attr_info, str2key):
    attr_tokens = list(attr_info.keys())

    for attr in attr_tokens:
        if 'alias' in attr_info[attr].keys():
            alias = attr_info[attr]['alias']
            attr_info[alias] = attr_info[attr].copy()
            attr_info[alias]['duplicated'] = attr

    attr_types = {}
    for attr in attr_info:
        attr_info[attr]['key'] = str2key (attr)
        if not WITHOUT_PRINT:
            print("%s: key of attribute name %s: 0x%x" % (TOOL_NAME, attr, attr_info[attr]['key'], ))

        attr_types[attr_info[attr]['type']] = 1
        if 'duplicated' in attr_info[attr].keys():
            if not WITHOUT_PRINT:
                print("%s: duplicated attr: %s" % (TOOL_NAME, attr_info[attr]['duplicated'], ))

    nr_attrs = len (attr_info)
    min_degree = nr_attrs * nr_attrs
    best_slots = 0
    for nr_slots in range (nr_attrs, 2 * nr_attrs):
        d = calc_hash_degree (attr_info, nr_slots)
        if d < min_degree:
            min_degree = d
            best_slots = nr_slots

    if not WITHOUT_PRINT:
        print("%s: find the best number of slots: %d" % (TOOL_NAME, best_slots))

    attr_table = []
    for i in range (0, best_slots):
        attr_table.append ({})

    last_slot = best_slots
    for attr in attr_info:
        idx = attr_info[attr]['key'] % best_slots
        if attr_table[idx]:
            while attr_table[idx]['next'] and attr_table[idx]['next'] > 0:
                idx = attr_table[idx]['next']
            attr_table[idx]['next'] = last_slot
            attr_table.append ({})
            attr_table[last_slot]['attr'] = attr
            attr_table[last_slot]['next'] = 0
            last_slot += 1
        else:
            attr_table[idx]['attr'] = attr
            attr_table[idx]['next'] = 0

    idx = 0
    for attr_slot in attr_table:
        if attr_slot:
            if not WITHOUT_PRINT:
                print("%s: slot #%d: %s; next: %d" % (TOOL_NAME, idx, attr_slot['attr'], attr_slot['next']))
        else:
            if not WITHOUT_PRINT:
                print("%s: slot #%d: no attr" % (TOOL_NAME, idx))

        idx += 1

    return attr_types, best_slots, attr_table

def write_attr_header (tmpl, dst, buf):
    lxb_temp = LXB.Temp(tmpl, dst)
    lxb_temp.pattern_append("%%PCHVML_ATTR_TYPE_ENUMS%%", '\n'.join(buf))

    lxb_temp.build()
    lxb_temp.save()

def write_static_attr_tables (tmpl_file, save_to, attr_info, best_slots, attr_table):
    lxb_temp = LXB.Temp(tmpl_file, save_to)
    lxb_temp.pattern_append("%%BEST_SLOTS%%", '{}'.format(best_slots))

    buf = []
    idx = 0
    for attr_slot in attr_table:
        if attr_slot:
            key = str2key_simple (attr_slot['attr'])
            buf.append("   // hash value of this attr: 0x%08x, slot index: %d" % (key, key % best_slots, ))
            buf.append("   { \"%s\", %s, %d}, // %d" % (attr_slot['attr'], make_attr_type (attr_info[attr_slot['attr']]['type']), attr_slot['next'], idx))
        else:
            buf.append("   { NULL, 0, 0}, // %d" % (idx))
        idx += 1
    lxb_temp.pattern_append("%%PCHVML_ATTR_STATIC_LIST_INDEX_RECORDS%%", '\n'.join(buf))

    lxb_temp.build()
    lxb_temp.save()


def write_attr_types (attr_types):
    buf = []

    buf.append ("    PCHVML_ATTR_TYPE_ORDINARY = 0,")

    for v in attr_types:
        buf.append ("    %s," % (make_attr_type (v), ))

    buf.append ("    PCHVML_ATTR_TYPE_LAST_ENTRY,")

    return buf

if __name__ == "__main__":
    if len(sys.argv) > 2 and sys.argv[2] == '--without-print':
        WITHOUT_PRINT = 1
    WITHOUT_PRINT = 0

    if len(sys.argv) < 2:
        raise Exception('expecting target path')

    this_path = "{}".format(os.path.dirname(sys.argv[0]))
    target_path = sys.argv[1];
    fn_inc = "{}/{}".format(this_path, SRC_FILE)
    try:
        f_inc = open(fn_inc, "r")
    except:
        if not WITHOUT_PRINT:
            print("%s: failed to open input file %s" % (TOOL_NAME, fn_inc, ))
        sys.exit(1)

    if not WITHOUT_PRINT:
        print("Scanning input file %s..." % SRC_FILE)
    attr_info = scan_src_file(f_inc)
    if attr_info is None:
        if not WITHOUT_PRINT:
            print("FAILED")
        sys.exit(3)
    f_inc.close()
    if not WITHOUT_PRINT:
        print("DONE")

    if not WITHOUT_PRINT:
        print("Generating static attr table...")
    attr_types, best_slots, attr_table = generate_static_attr_table (attr_info, str2key_simple)
    if not WITHOUT_PRINT:
        print("DONE")

    tmpl = "{}/data/{}.in".format(this_path, INC_FILE)
    dst  = "{}/hvml_{}".format(target_path, INC_FILE)

    if not WITHOUT_PRINT:
        print("Writting HVML static attr table to dst file %s..." % dst)
    try:
        write_static_attr_tables (tmpl, dst, attr_info, best_slots, attr_table)
    except:
        if not WITHOUT_PRINT:
            print("FAILED")
        traceback.print_exc()
        sys.exit(13)

    if not WITHOUT_PRINT:
        print("DONE")

    tmpl = "{}/data/{}.in".format(this_path, H_FILE)
    dst  = "{}/hvml_{}".format(target_path, H_FILE)

    if not WITHOUT_PRINT:
        print("Writting HVML attr types to dst file %s..." %dst)
    try:
        buf = write_attr_types (attr_types)
        write_attr_header (tmpl, dst, buf)
    except:
        if not WITHOUT_PRINT:
            print("FAILED")
        traceback.print_exc()
        sys.exit(13)
    if not WITHOUT_PRINT:
        print("DONE")

    sys.exit(0)

