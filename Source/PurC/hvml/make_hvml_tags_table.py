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
Make HVML tags table:
    1. Read 'data/hvmltags.txt' file.
    5. Generate the myhvml_tag_static_list_t tables (two tables; one for 64bit, the other for 32bit).
    5. Write code to 'tag_static_list.inc'.
"""

import os, sys
import time
import re

TOOL_NAME="make_hvml_tags_table.py"
SRC_FILE="data/hvmltags.txt"
HVMLTAGSTABLE_FILE="tag_static_list.inc"

RE_START_WITH_VALUES = re.compile(r"^\s+values:")
def start_with_values(line):
    if RE_START_WITH_VALUES.match(line) == None:
        return False
    return True

RE_START_WITH_INITIAL = re.compile(r"^\s+initial:")
def start_with_initial(line):
    if RE_START_WITH_INITIAL.match(line) == None:
        return False
    return True

RE_START_WITH_INHERITED = re.compile(r"^\s+inherited:")
def start_with_inherited(line):
    if RE_START_WITH_INHERITED.match(line) == None:
        return False
    return True

RE_START_WITH_APPLIESTO = re.compile(r"^\s+appliesto:")
def start_with_appliesto(line):
    if RE_START_WITH_APPLIESTO.match(line) == None:
        return False
    return True

RE_START_WITH_ARRAYSIZE = re.compile(r"^\s+arraysize:")
def start_with_arraysize(line):
    if RE_START_WITH_ARRAYSIZE.match(line) == None:
        return False
    return True

RE_START_WITH_KIND = re.compile(r"^\s+kind:")
def start_with_kind(line):
    if RE_START_WITH_KIND.match(line) == None:
        return False
    return True

RE_START_WITH_STATE = re.compile(r"^\s+state:")
def start_with_state(line):
    if RE_START_WITH_STATE.match(line) == None:
        return False
    return True

RE_START_WITH_CATEGORIES = re.compile(r"^\s+categories:")
def start_with_categories(line):
    if RE_START_WITH_CATEGORIES.match(line) == None:
        return False
    return True

RE_START_WITH_SPACE = re.compile(r"^\s")
def start_with_space(line):
    if RE_START_WITH_SPACE.match(line) == None:
        return False
    return True

RE_IS_FLAG = re.compile(r"^&\S+$")
def is_flag(value):
    if RE_IS_FLAG.match(value) is not None:
        return True
    return False

RE_IS_VARIABLE = re.compile(r"^<\S+>$")
def is_variable(value):
    if RE_IS_VARIABLE.match(value) is not None:
        return True
    return False

def is_keyword(value):
    if not is_flag(value) and not is_variable(value):
        return True
    return False

def do_subexpand(def_info, definition):
    values = definition.split()
    if len(values) > 0:
        for value in values:
            if value in def_info:
                definition = definition.replace(value, def_info[value])
                return do_subexpand(def_info, definition)

    return definition

def expand_definition(def_info, token, definition):
    token = token.strip()
    word = token.strip("<>")
    definition = definition.replace('%', word)
    def_info[token] = do_subexpand(def_info, definition)

def get_value_list(line):
    fragments = line.split(":")

    if len(fragments) == 2:
        values = fragments[1].strip()
        value_list = values.split()
        if value_list is None or len(value_list) == 0:
            return None
        return value_list

    return None

def get_value(line):
    fragments = line.split(":")

    if len(fragments) == 2:
        value = fragments[1].strip()
        if len(value) == 0:
            return None
        return value

    return None

def set_value_list(tag_info, tag_token, def_info, values):
    value_list = values.split()
    tag_info[tag_token]['org_values'] = value_list

    if len(value_list) > 0:
        for value in value_list:
            if value in def_info:
                values = values.replace(value, def_info[value])

    value_list = values.split()
    tag_info[tag_token]['values'] = value_list

def set_category_list(tag_info, tag_token, def_info, categories):
    category_list = categories.split()
    tag_info[tag_token]['org_categories'] = category_list

    if len(category_list) > 0:
        for category in category_list:
            if category in def_info:
                categories = categories.replace(category, def_info[category])

    category_list = categories.split()
    tag_info[tag_token]['categories'] = category_list

def scan_src_file(fsrc):
    def_info = {}
    tag_info = {}

    tag_token = ""
    value_line = ""
    line_no = 1
    org_line = fsrc.readline()
    while org_line:
        stripped_line = org_line.strip()
        if stripped_line == "" or stripped_line[0] == '#':
            line_no = line_no + 1
            org_line = fsrc.readline()
            continue

        tokens = stripped_line.split(":")
        if len (tokens) > 0 and is_variable(tokens[0]):
            expand_definition (def_info, tokens[0], tokens[1])
        else:
            if start_with_values (org_line):
                values = get_value(stripped_line)
                if values is None:
                    print("scan_src_file (Line %d): value list expected (%s)" % (line_no, stripped_line, ))
                    return None
                else:
                    set_value_list(tag_info, tag_token, def_info, values)

            elif start_with_initial (org_line):
                initial_value = get_value(stripped_line)
                if initial_value is None:
                    print("scan_src_file (Line %d): initial value expected (%s)" % (line_no, stripped_line, ))
                    return None
                else:
                    tag_info[tag_token]['initial'] = initial_value

            elif start_with_inherited (org_line):
                inherited_value = get_value(stripped_line)
                if inherited_value is None or (inherited_value != "no" and inherited_value != "yes"):
                    print("scan_src_file (Line %d): inherited value (yes/no) expected (%s)" % (line_no, stripped_line, ))
                    return None
                else:
                    tag_info[tag_token]['inherited'] = inherited_value

            elif start_with_appliesto (org_line):
                appliesto_value = get_value(stripped_line)
                if appliesto_value is None:
                    print("scan_src_file (Line %d): appliesto value expected (%s)" % (line_no, stripped_line, ))
                    return None
                else:
                    tag_info[tag_token]['appliesto'] = appliesto_value

            elif start_with_arraysize (org_line):
                arraysize_value = get_value(stripped_line)
                if arraysize_value is None:
                    print("scan_src_file (Line %d): arraysize value expected (%s)" % (line_no, stripped_line, ))
                    return None
                else:
                    tag_info[tag_token]['arraysize'] = arraysize_value

            elif start_with_kind (org_line):
                kind_value = get_value(stripped_line)
                if kind_value is None:
                    print("scan_src_file (Line %d): kind value expected (%s)" % (line_no, stripped_line, ))
                    return None
                else:
                    tag_info[tag_token]['kind'] = kind_value

            elif start_with_state (org_line):
                state_value = get_value(stripped_line)
                if state_value is None:
                    print("scan_src_file (Line %d): state value expected (%s)" % (line_no, stripped_line, ))
                    return None
                else:
                    tag_info[tag_token]['state'] = state_value

            elif start_with_categories (org_line):
                categories = get_value(stripped_line)
                if categories is None:
                    print("scan_src_file (Line %d): value list expected (%s)" % (line_no, stripped_line, ))
                    return None
                else:
                    set_category_list(tag_info, tag_token, def_info, categories)

            elif not start_with_space (org_line):
                tag_token = stripped_line
                if tag_token in tag_info:
                    print("scan_src_file (Line %d): duplicated property name (%s)" % (line_no, stripped_line, ))
                    return None
                tag_info[tag_token] = {}
            else:
                print("scan_src_file (Line %d): syntax error %s (%s)" % (line_no, tag_token, stripped_line, ))
                return None

        line_no = line_no + 1
        org_line = fsrc.readline()

    return tag_info

FNV_PRIME_64B = 0x100000001b3
FNV_INIT_64B  = 0xcbf29ce484222325

FNV_PRIME_32B = 0x01000193
FNV_INIT_32B  = 0x811c9dc5

def str2key_gcc_64b (_str):
    _bytes = _str.encode()

    hval = FNV_INIT_64B;
    for c in _bytes:
        hval ^= c
        hval += (hval << 1) + (hval << 4) + (hval << 5) + (hval << 7) + (hval << 8) + (hval << 40)

    return hval & 0xFFFFFFFFFFFFFFFF

def str2key_gcc_32b (_str):

    _bytes = _str.encode()
    hval = FNV_INIT_32B;
    for c in _bytes:
        hval ^= c
        hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24)

    return hval & 0xFFFFFFFF

def str2key_64b (_str):

    _bytes = _str.encode()
    hval = FNV_INIT_64B;

    for c in _bytes:
        hval ^= c
        hval *= FNV_PRIME_64B

    return hval & 0xFFFFFFFFFFFFFFFF

def str2key_32b (_str):
    _bytes = _str.encode()

    hval = FNV_INIT_32B;
    for c in _bytes:
        hval ^= c
        hval *= FNV_PRIME_32B

    return hval & 0xFFFFFFFF

def calc_hash_degree (tag_info, nr_slots):

    degrees = []
    for i in range (0, nr_slots):
        degrees.append (0)

    min_degree = nr_slots
    max_degree = 0

    for tag in tag_info:
        slot_idx = tag_info[tag]['key'] % nr_slots
        degrees [slot_idx] += 1

    nr_zeros = 0
    for d in degrees:
        if d > max_degree:
            max_degree = d
        if d < min_degree:
            min_degree = d
        if d == 0:
            nr_zeros += 1

    print("%s: min/max hash degree: (%d, %d) / %d; zero slots: %d; %d" % (TOOL_NAME, min_degree, max_degree, nr_slots, nr_zeros, nr_zeros * max_degree))
    return nr_zeros * max_degree

def make_tag_id(tag_token):
    tag_id = tag_token.upper()
    tag_id = tag_id.replace('-', '_')
    tag_id = tag_id.replace('!', '_')

    return "MyHVML_TAG_" + tag_id;

def make_state_id(state_token):
    state_id = state_token.upper()

    return "MyHVML_TOKENIZER_STATE_" + state_id;

def make_category_id(category_token):
    category_id = category_token.upper()

    return "MyHVML_TAG_CATEGORIES_" + category_id;

def make_categories_value (categories_list):

    value = ""
    idx = 0
    for c in categories_list:
        if idx > 0:
            value += " | "
        value += make_category_id (c)
        idx += 1

    return value

def generate_static_tag_table (tag_info, str2key):
    nr_tags = len (tag_info)
    for tag in tag_info:
        tag_info[tag]['key'] = str2key (tag)
        print("%s: key of string %s: 0x%x" % (TOOL_NAME, tag, tag_info[tag]['key'], ))

    min_degree = nr_tags * nr_tags
    best_slots = 0
    for nr_slots in range (nr_tags, 2 * nr_tags):
        d = calc_hash_degree (tag_info, nr_slots)
        if d < min_degree:
            min_degree = d
            best_slots = nr_slots

    print("%s: find the best number of slots: %d" % (TOOL_NAME, best_slots))

    tag_table = []
    for i in range (0, best_slots):
        tag_table.append ({})

    last_slot = best_slots
    for tag in tag_info:
        idx = tag_info[tag]['key'] % best_slots
        if tag_table[idx]:
            tag_table[idx]['next'] = last_slot
            tag_table.append ({})
            tag_table[last_slot]['tag'] = tag
            tag_table[last_slot]['next'] = 0
            last_slot += 1
        else:
            tag_table[idx]['tag'] = tag
            tag_table[idx]['next'] = 0

    idx = 0
    for tag_slot in tag_table:
        if tag_slot:
            print("%s: slot #%d: %s; next: %d" % (TOOL_NAME, idx, tag_slot['tag'], tag_slot['next']))
        else:
            print("%s: slot #%d: no tag" % (TOOL_NAME, idx))

        idx += 1

    return best_slots, tag_table

def make_property_id(property_token):
    property_id = property_token.upper()
    property_id = property_id.replace('-', '_')

    return "PID_" + property_id;

def make_value_type_id(type_token):
    type_id = type_token.strip('<>')
    type_id = type_id.replace('-', '_')
    type_id = type_id.upper()
    return "PVT_" + type_id;

def make_value_keyword_id(keyword_token):
    keyword_id = keyword_token.upper()
    keyword_id = keyword_id.replace('-', '_')
    return "PVK_" + keyword_id;

def make_flag_id(tag_token, value_token):
    tag_id = tag_token.replace('-', '_')
    tag_id = tag_id.upper()

    value_id = value_token.replace('&', '_')
    value_id = value_id.replace('-', '_')
    value_id = value_id.upper()

    return "PVF_" + tag_id + value_id;

def make_pv_checker_name(tag_token):
    name = tag_token.lower()
    name = name.replace('-', '_')

    return "pvc_" + name;

def make_common_checker_name(value_token):
    name = value_token.strip('<>')
    name = name.replace('-', '_')
    name = name.lower()

    return "check_" + name;

def write_static_tag_tables (fout, tag_info, best_slots_64b, tag_table_64b, best_slots_32b, tag_table_32b):
    fout.write ("/*\n")
    fout.write ("** Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>\n")
    fout.write ("**\n")
    fout.write ("** This file is a part of Purring Cat 2, a HVML parser and interpreter.\n")
    fout.write ("** \n")
    fout.write ("** This program is free software: you can redistribute it and/or modify\n")
    fout.write ("** it under the terms of the GNU Lesser General Public License as published by\n")
    fout.write ("** the Free Software Foundation, either version 3 of the License, or\n")
    fout.write ("** (at your option) any later version.\n")
    fout.write ("**\n")
    fout.write ("** This program is distributed in the hope that it will be useful,\n")
    fout.write ("** but WITHOUT ANY WARRANTY; without even the implied warranty of\n")
    fout.write ("** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n")
    fout.write ("** GNU Lesser General Public License for more details.\n")
    fout.write ("**\n")
    fout.write ("** You should have received a copy of the GNU Lesser General Public License\n")
    fout.write ("** along with this program.  If not, see <https://www.gnu.org/licenses/>.\n")
    fout.write ("**\n")
    fout.write ("** Author: Vincent Wei <https://github.com/VincentWei>\n")
    fout.write ("*/\n")
    fout.write ("\n")
    fout.write ("// NOTE\n")
    fout.write ("// This file is auto-generated by using '%s'.\n" % (TOOL_NAME, ))
    fout.write ("// Please take care when you modify this file mannually.\n")
    fout.write ("\n")

    fout.write ("static const myhvml_tag_context_t myhvml_tag_base_list[MyHVML_TAG_LAST_ENTRY] =\n")
    fout.write ("{\n")

    tag_tokens = list(tag_info.keys())
    #tag_tokens.sort()

    for tag in tag_tokens:
        fout.write ("    { %s, \"%s\", %d, %s,\n" % (make_tag_id (tag), tag, len (tag), make_state_id (tag_info[tag]['state']), ))
        fout.write ("        %s },\n" % (make_categories_value (tag_info[tag]['categories']), ))

    fout.write ("};\n")
    fout.write ("\n")

    fout.write ("#if SIZEOF_PTR == 8\n")
    fout.write ("\n")
    fout.write ("#define MyHVML_BASE_STATIC_SIZE %d\n" % (best_slots_64b, ))
    fout.write ("\n")
    fout.write ("static const myhvml_tag_static_list_t myhvml_tag_static_list_index[] =\n")
    fout.write ("{\n")

    for tag_slot in tag_table_64b:
        if tag_slot:
            key = str2key_64b (tag_slot['tag'])
            fout.write ("   // hash value of this tag: 0x%016x, slot index: %d\n" % (key, key % best_slots_64b, ))
            fout.write ("   { &myhvml_tag_base_list[%s], %d},\n" % (make_tag_id (tag_slot['tag']), tag_slot['next'], ))
        else:
            fout.write ("   { NULL, 0},\n")

    fout.write ("};\n")
    fout.write ("\n")
    fout.write ("#else   /* SIZEOF_PTR == 4 */\n")
    fout.write ("\n")
    fout.write ("#define MyHVML_BASE_STATIC_SIZE %d\n" % (best_slots_32b, ))
    fout.write ("\n")
    fout.write ("static const myhvml_tag_static_list_t myhvml_tag_static_list_index[] =\n")
    fout.write ("{\n")

    for tag_slot in tag_table_32b:
        if tag_slot:
            key = str2key_32b (tag_slot['tag'])
            fout.write ("   // hash value of this tag: 0x%016x, slot index: %d\n" % (key, key % best_slots_32b, ))
            fout.write ("   { &myhvml_tag_base_list[%s], %d},\n" % (make_tag_id (tag_slot['tag']), tag_slot['next'], ))
        else:
            fout.write ("   { NULL, 0},\n")

    fout.write ("};\n")
    fout.write ("\n")
    fout.write ("#endif  /* SIZEOF_PTR == 4 */\n")
    fout.write ("\n")

def write_tag_ids (fout, tag_info):

    tag_tokens = list(tag_info.keys())

    fout.write ("enum myhvml_tags {\n")
    idx = 0
    for tag in tag_tokens:
        if idx == 0:
            fout.write ("    %s = 0, \n" % (make_tag_id (tag), ))
        else:
            fout.write ("    %s, \n" % (make_tag_id (tag), ))

        if idx == 1:
            first_tag = tag
        last_tag = tag

        idx += 1

    fout.write ("    MyHVML_TAG_FIRST_ENTRY = %s,\n" % (make_tag_id (first_tag), ))
    fout.write ("    MyHVML_TAG_LAST_ENTRY  = %s + 1,\n" % (make_tag_id (last_tag), ))
    fout.write ("};\n")
    fout.write ("\n")

if __name__ == "__main__":
    this_path = "{}".format(os.path.dirname(sys.argv[0]))
    try:
        fsrc = open("{}/{}".format(this_path, SRC_FILE), "r")
    except:
        print("%s: failed to open input file %s" % (TOOL_NAME, SRC_FILE, ))
        sys.exit(1)

    print("Scanning input file %s..." % SRC_FILE)
    tag_info = scan_src_file(fsrc)
    if tag_info is None:
        print("FAILED")
        sys.exit(3)
    fsrc.close()
    print("DONE")

    print("Generating static tag table for 64bit...")
    best_slots_64b, tag_table_64b = generate_static_tag_table (tag_info, str2key_64b)

    print("Generating static tag table for 32bit...")
    best_slots_32b, tag_table_32b = generate_static_tag_table (tag_info, str2key_32b)

    print("DONE")

    try:
        fdst = open("{}/{}".format(this_path, HVMLTAGSTABLE_FILE), "w")
    except:
        print("%s: failed to open output file %s" % (TOOL_NAME, HVMLTAGSTABLE_FILE, ))
        sys.exit(12)

    print("Writting HVML static tag table to dst file %s..." % HVMLTAGSTABLE_FILE)
    try:
        write_static_tag_tables (fdst, tag_info, best_slots_64b, tag_table_64b, best_slots_32b, tag_table_32b)
    except:
        print("FAILED")
        traceback.print_exc()
        sys.exit(13)
    fdst.close()
    print("DONE")

    print("Writting HVML tag identifiers to standard output...")
    try:
        write_tag_ids (sys.stdout, tag_info)
    except:
        print("FAILED")
        traceback.print_exc()
        sys.exit(13)
    print("DONE")

    sys.exit(0)

