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
    1. Read 'data/hvmlattrs.txt' file.
    5. Generate the myhvml_attr_static_list_t table.
    5. Write code to 'attr_static_list.inc'.
"""

import os, sys
import time
import re

WITHOUT_PRINT = 0

TOOL_NAME="make_hvml_attrs_table.py"
SRC_FILE="data/hvmlattrs.txt"
HVMLATTRSTABLE_FILE="hvml_attr_static_list.inc"

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

def set_value_list(attr_info, attr_token, def_info, values):
    value_list = values.split()
    attr_info[attr_token]['org_values'] = value_list

    if len(value_list) > 0:
        for value in value_list:
            if value in def_info:
                values = values.replace(value, def_info[value])

    value_list = values.split()
    attr_info[attr_token]['values'] = value_list

def set_category_list(attr_info, attr_token, def_info, categories):
    category_list = categories.split()
    attr_info[attr_token]['org_categories'] = category_list

    if len(category_list) > 0:
        for category in category_list:
            if category in def_info:
                categories = categories.replace(category, def_info[category])

    category_list = categories.split()
    attr_info[attr_token]['categories'] = category_list

def scan_src_file(fsrc):
    def_info = {}
    attr_info = {}

    attr_token = ""
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
                    if not WITHOUT_PRINT:
                        print("scan_src_file (Line %d): value list expected (%s)" % (line_no, stripped_line, ))
                    return None
                else:
                    set_value_list(attr_info, attr_token, def_info, values)

            elif start_with_initial (org_line):
                initial_value = get_value(stripped_line)
                if initial_value is None:
                    if not WITHOUT_PRINT:
                        print("scan_src_file (Line %d): initial value expected (%s)" % (line_no, stripped_line, ))
                    return None
                else:
                    attr_info[attr_token]['initial'] = initial_value

            elif start_with_inherited (org_line):
                inherited_value = get_value(stripped_line)
                if inherited_value is None or (inherited_value != "no" and inherited_value != "yes"):
                    if not WITHOUT_PRINT:
                        print("scan_src_file (Line %d): inherited value (yes/no) expected (%s)" % (line_no, stripped_line, ))
                    return None
                else:
                    attr_info[attr_token]['inherited'] = inherited_value

            elif start_with_appliesto (org_line):
                appliesto_value = get_value(stripped_line)
                if appliesto_value is None:
                    if not WITHOUT_PRINT:
                        print("scan_src_file (Line %d): appliesto value expected (%s)" % (line_no, stripped_line, ))
                    return None
                else:
                    attr_info[attr_token]['appliesto'] = appliesto_value

            elif start_with_arraysize (org_line):
                arraysize_value = get_value(stripped_line)
                if arraysize_value is None:
                    if not WITHOUT_PRINT:
                        print("scan_src_file (Line %d): arraysize value expected (%s)" % (line_no, stripped_line, ))
                    return None
                else:
                    attr_info[attr_token]['arraysize'] = arraysize_value

            elif start_with_state (org_line):
                state_value = get_value(stripped_line)
                if state_value is None:
                    if not WITHOUT_PRINT:
                        print("scan_src_file (Line %d): state value expected (%s)" % (line_no, stripped_line, ))
                    return None
                else:
                    attr_info[attr_token]['state'] = state_value

            elif start_with_categories (org_line):
                categories = get_value(stripped_line)
                if categories is None:
                    if not WITHOUT_PRINT:
                        print("scan_src_file (Line %d): value list expected (%s)" % (line_no, stripped_line, ))
                    return None
                else:
                    set_category_list(attr_info, attr_token, def_info, categories)

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
        org_line = fsrc.readline()

    return attr_info

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

def make_attr_id(attr_token):
    attr_id = attr_token.upper()
    attr_id = attr_id.replace('-', '_')
    attr_id = attr_id.replace('!', '_')

    return "MyHVML_ATTR_TYPE_" + attr_id;

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

def generate_static_attr_table (attr_info, str2key):
    attr_tokens = list(attr_info.keys())

    for attr in attr_tokens:
        if 'alias' in attr_info[attr].keys():
            alias = attr_info[attr]['alias']
            attr_info[alias] = attr_info[attr].copy()
            attr_info[alias]['duplicated'] = attr

    attr_ids = []
    for attr in attr_info:
        attr_info[attr]['key'] = str2key (attr)
        if not WITHOUT_PRINT:
            print("%s: key of attribute name %s: 0x%x" % (TOOL_NAME, attr, attr_info[attr]['key'], ))

        if 'duplicated' in attr_info[attr].keys():
            attr_info[attr]['id'] = attr_info[attr]['type'] + '_' + attr_info[attr]['duplicated']
            if not WITHOUT_PRINT:
                print("%s: duplicated attr: %s" % (TOOL_NAME, attr_info[attr]['duplicated'], ))
        else:
            attr_info[attr]['id'] = attr_info[attr]['type'] + '_' + attr
            attr_ids.append (attr_info[attr]['id'])
            if not WITHOUT_PRINT:
                print("%s: new attr id: %s" % (TOOL_NAME, attr_info[attr]['id'], ))
    attr_ids.sort()

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

    return attr_ids, best_slots, attr_table

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

def make_flag_id(attr_token, value_token):
    attr_id = attr_token.replace('-', '_')
    attr_id = attr_id.upper()

    value_id = value_token.replace('&', '_')
    value_id = value_id.replace('-', '_')
    value_id = value_id.upper()

    return "PVF_" + attr_id + value_id;

def make_pv_checker_name(attr_token):
    name = attr_token.lower()
    name = name.replace('-', '_')

    return "pvc_" + name;

def make_common_checker_name(value_token):
    name = value_token.strip('<>')
    name = name.replace('-', '_')
    name = name.lower()

    return "check_" + name;

def write_static_attr_tables (fout, attr_ids, attr_info, best_slots, attr_table):
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

#    fout.write ("static const myhvml_attr_context_t myhvml_attr_base_list[MyHVML_ATTR_TYPE_LAST_ENTRY] =\n")
#    fout.write ("{\n")
#
#    for attr_id in attr_ids:
#        fragments = attr_id.split("_")
#        attr = fragments[1]
#        fout.write ("    { %s, \"%s\", %d },\n" % (make_attr_id(attr_info[attr]['id']), attr, len (attr), ))
#
#    fout.write ("};\n")
#    fout.write ("\n")

    fout.write ("#define MyHVML_ATTR_STATIC_SIZE %d\n" % (best_slots, ))
    fout.write ("\n")

    fout.write ("typedef struct myhvml_attr_static_list {\n");
    fout.write ("    const char* name;\n");
    fout.write ("    short type;\n");
    fout.write ("    short next;\n");
    fout.write ("} myhvml_attr_static_list_t;\n");
    fout.write ("\n")

    fout.write ("static const myhvml_attr_static_list_t myhvml_attr_static_list_index[] =\n")
    fout.write ("{\n")

    for attr_slot in attr_table:
        if attr_slot:
            key = str2key_simple (attr_slot['attr'])
            fout.write ("   // hash value of this attr: 0x%08x, slot index: %d\n" % (key, key % best_slots, ))
            fout.write ("   { \"%s\", %s, %d},\n" % (attr_slot['attr'], make_attr_id (attr_info[attr_slot['attr']]['id']), attr_slot['next'], ))
        else:
            fout.write ("   { NULL, 0, 0},\n")

    fout.write ("};\n")
    fout.write ("\n")

def write_attr_ids (fout, attr_ids, attr_info):

    fout.write ("enum myhvml_attr_types {\n")
    fout.write ("    MyHVML_ATTR_TYPE_ORDINARY = 0,\n")

    idx = 0
    for attr in attr_ids:
        fout.write ("    %s, \n" % (make_attr_id (attr), ))

        if idx == 0:
            first_tag = attr
        last_tag = attr
        idx += 1

    fout.write ("    MyHVML_ATTR_TYPE_FIRST_ENTRY = %s,\n" % (make_attr_id (first_tag), ))
    fout.write ("    MyHVML_ATTR_TYPE_LAST_ENTRY  = %s + 1,\n" % (make_attr_id (last_tag), ))
    fout.write ("};\n")
    fout.write ("\n")

if __name__ == "__main__":
    if len(sys.argv) > 2 and sys.argv[2] == '--without-print':
        WITHOUT_PRINT = 1

    if len(sys.argv) < 2:
        raise Exception('expecting target path')

    this_path = "{}".format(os.path.dirname(sys.argv[0]))
    target_path = sys.argv[1];
    fn = "{}/{}".format(this_path, SRC_FILE)
    try:
        fsrc = open(fn, "r")
    except:
        if not WITHOUT_PRINT:
            print("%s: failed to open input file %s" % (TOOL_NAME, fn, ))
        sys.exit(1)

    if not WITHOUT_PRINT:
        print("Scanning input file %s..." % SRC_FILE)
    attr_info = scan_src_file(fsrc)
    if attr_info is None:
        if not WITHOUT_PRINT:
            print("FAILED")
        sys.exit(3)
    fsrc.close()
    if not WITHOUT_PRINT:
        print("DONE")

    if not WITHOUT_PRINT:
        print("Generating static attr table...")
    attr_ids, best_slots, attr_table = generate_static_attr_table (attr_info, str2key_simple)
    if not WITHOUT_PRINT:
        print("DONE")

    fn = "{}/{}".format(target_path, HVMLATTRSTABLE_FILE)
    try:
        fdst = open(fn, "w")
    except:
        if not WITHOUT_PRINT:
            print("%s: failed to open output file %s" % (TOOL_NAME, fn, ))
        sys.exit(12)

    if not WITHOUT_PRINT:
        print("Writting HVML static attr table to dst file %s..." % HVMLATTRSTABLE_FILE)
    try:
        write_static_attr_tables (fdst, attr_ids, attr_info, best_slots, attr_table)
    except:
        if not WITHOUT_PRINT:
            print("FAILED")
        traceback.print_exc()
        sys.exit(13)
    fdst.close()
    if not WITHOUT_PRINT:
        print("DONE")

    if not WITHOUT_PRINT:
        print("Writting HVML attr identifiers to standard output...")
    try:
        if not WITHOUT_PRINT:
            write_attr_ids (sys.stdout, attr_ids, attr_info)
    except:
        if not WITHOUT_PRINT:
            print("FAILED")
        traceback.print_exc()
        sys.exit(13)
    if not WITHOUT_PRINT:
        print("DONE")

    sys.exit(0)

