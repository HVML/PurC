#
# Copyright 2021 FMSoft (<https://www.fmsoft.cn>)
# Copyright 2018-2020 Alexander Borisov
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#
# My eyes bleed when I see this code.
# It works correctly, but it needs refactoring!
#

import json
import sys, re, os
import hashlib

# Find and append run script run dir to module search path
ABS_PATH = os.path.dirname(os.path.abspath(__file__))
sys.path.append("{}/../Scripts/".format(ABS_PATH))

import LXB
import interfaces

WITHOUT_PRINT = 0

def convert_to_c(tag_name):
    return re.sub(r"[^a-zA-Z0-9_]", "_", tag_name)

def computeMD5hash(my_string):
    m = hashlib.md5()
    m.update(my_string.encode('utf-8'))
    return m.hexdigest()

class Tags:
    tag_prefix = "pchtml_tag"
    tag_res_data = "pchtml_tag_res_data_default"
    tag_res_data_upper = "pchtml_tag_res_data_upper_default"
    ns_prefix = "pchtml_ns"
    ns_res_data = "pchtml_ns_res_data"
    ns_res_prefix_data = "pchtml_ns_prefix_res_data"
    cat_prefix = "pchtml_html_tag_category"
    cat_empty = "PCHTML_HTML_TAG_CATEGORY__UNDEF"
    creation_interface = "pcedom_interface_constructor_f"
    deletion_interface = "pcedom_interface_destructor_f"

    def __init__(self, json_tags, json_interfaces):
        self.interfaces = interfaces.Interfaces(json_interfaces)

        self.tags = json.load(open(json_tags))
        self.make_tags()

    def make_tags(self):
        shs = {}
        shs_list = []

        enum = {}
        enum_list = []

        ns_shs_list = []
        ns_shs_link_list = []

        # namespace
        self.ns = self.tags["namespaces"]
        self.ns_list = list(self.ns.keys())
        self.ns_list.sort(key = lambda entr: (100000 if "sort" not in self.ns[entr] else self.ns[entr]["sort"], entr))

        interfaces = self.interfaces

        for idx in range(0, len(self.ns_list)):
            self.ns[ self.ns_list[idx] ]["c_name"] = "{}_{}".format(self.ns_prefix.upper(),
                                                                    convert_to_c(self.ns_list[idx]).upper())
            self.ns[ self.ns_list[idx] ]["id"] = idx

            link = self.ns[self.ns_list[idx]]["link"]

            if len(link) == 0:
                link = self.ns[self.ns_list[idx]]["name"]

            ns_shs_list.append({"key": self.ns_list[idx].lower(), "value": "(void *) &{}[{}]".format(self.ns_res_prefix_data, idx)})
            ns_shs_link_list.append({"key": link.lower(), "value": "(void *) &{}[{}]".format(self.ns_res_data, idx)})

        self.ns_hash = self.ns_make_hash(self.ns_list)
        self.ns_shs_list = ns_shs_list
        self.ns_shs_link_list = ns_shs_link_list

        # shs data and enum
        data = self.tags["data"]

        for ns in data:
            for tag in data[ns]:
                if "to" not in data[ns][tag]:
                    enum_name = "{}_{}".format(self.tag_prefix.upper(),
                                               convert_to_c(tag).upper())
                else:
                    enum_name = "{}_{}".format(self.tag_prefix.upper(),
                                               convert_to_c(data[ns][tag]["to"]).upper())

                shs[tag] = {"key": tag}

                if enum_name not in enum:
                    enum[enum_name] = {"ns": [], "name": tag, "c_name": enum_name, "interface": [], "interface_del": [], "fixname": []}

                    for idx in self.ns_list:
                        default = self.ns[idx]["default"]
                        interface = interfaces.make_function_create_name(default["interface_type"], default["interface"])
                        interface_del = interfaces.make_function_destroy_name(default["interface_type"], default["interface"])

                        enum[enum_name]["ns"].append([])
                        enum[enum_name]["interface"].append(interface)
                        enum[enum_name]["interface_del"].append(interface_del)
                        enum[enum_name]["fixname"].append("NULL")


                ns_id = self.ns[ns]["id"]
                cat = data[ns][tag]["cat"]

                for cat_name in cat:
                    enum[enum_name]["ns"][ns_id].append(
                        "{}_{}".format(self.cat_prefix.upper(), convert_to_c(cat_name).upper())
                    )

                    enum[enum_name]["ns"][ns_id].sort()

                interface = data[ns][tag]["interface"]
                interface_type = data[ns][tag]["interface_type"]

                enum[enum_name]["interface"][ns_id] = interfaces.make_function_create_name(interface_type, interface);
                enum[enum_name]["interface_del"][ns_id] = interfaces.make_function_destroy_name(interface_type, interface);

                if "fixname" in data[ns][tag]:
                    enum[enum_name]["fixname"][ns_id] = data[ns][tag]["fixname"]

                if "sort" in data[ns][tag]:
                    enum[enum_name]["sort"] = data[ns][tag]["sort"]
                elif "sort" not in enum[enum_name]:
                    enum[enum_name]["sort"] = 1000000

        for key in shs:
            shs_list.append(shs[key])

        for key in enum:
            enum_list.append(enum[key])

            for ns_entry in enum[key]["ns"]:
                ns_entry.sort()

        shs_list.sort(key = lambda entr: entr["key"])
        enum_list.sort(key = lambda entr: (entr["sort"], entr["c_name"]))

        for idx in range(0, len(enum_list)):
            shs[ enum_list[idx]["name"] ]["value"] = "(void *) &{}[{}]".format(self.tag_res_data, idx)

        self.shs_list = shs_list

        self.enum = enum
        self.enum_list = enum_list
        self.enum_hash = self.enum_make_hash(enum_list)

        self.ns_last_entry_name = convert_to_c("{}__LAST_ENTRY".format(self.ns_prefix)).upper()
        self.tag_last_entry_name = convert_to_c("{}__LAST_ENTRY".format(self.tag_prefix)).upper()

        return shs_list

    def tag_data_create_default(self):
        res = LXB.Res("pchtml_tag_data_t", self.tag_res_data, False, self.tag_last_entry_name)
        res_upper = LXB.Res("pchtml_tag_data_t", self.tag_res_data_upper, False, self.tag_last_entry_name)

        for entry in self.enum_list:
            # res.append("{{{{.u.long_str = (unsigned char *) \"{}\", .length = {}, .next = NULL}},\n     (const unsigned char *) \"{}\", {}, 1, true}}".format(entry["name"], len(entry["name"]), entry["name"].upper(), entry["c_name"]))

            if len(entry["name"]) > 16:
                res.append("{{{{.u.long_str = (unsigned char *) \"{}\", .length = {}, .next = NULL}}, {}, 1, true}}".format(entry["name"], len(entry["name"]), entry["c_name"]))
                res_upper.append("{{{{.u.long_str = (unsigned char *) \"{}\", .length = {}, .next = NULL}}, {}, 1, true}}".format(entry["name"].upper(), len(entry["name"]), entry["c_name"]))
            else:
                res.append("{{{{.u.short_str = \"{}\", .length = {}, .next = NULL}}, {}, 1, true}}".format(entry["name"], len(entry["name"]), entry["c_name"]))
                res_upper.append("{{{{.u.short_str = \"{}\", .length = {}, .next = NULL}}, {}, 1, true}}".format(entry["name"].upper(), len(entry["name"]), entry["c_name"]))

        return [res.create(1, True), res_upper.create(1, True)]

    def tag_data_create_html(self):
        result = []
        cats = LXB.Res("pchtml_html_tag_category_t", "pchtml_html_tag_res_cats", True, [self.tag_last_entry_name, self.ns_last_entry_name])
        constructor = LXB.Res("pcedom_interface_constructor_f", "pchtml_html_interface_res_constructors", True, [self.tag_last_entry_name, self.ns_last_entry_name])
        destructor = LXB.Res("pcedom_interface_destructor_f", "pchtml_html_interface_res_destructor", True, [self.tag_last_entry_name, self.ns_last_entry_name])
        svg_fixname = LXB.Res("pchtml_html_tag_fixname_t", "pchtml_html_tag_res_fixname_svg", True, [self.tag_last_entry_name])

        fixname = []

        for idx in range(0, len(self.ns_list)):
            fixname.append([])

        for entry in self.enum_list:
            ns = ["        "]
            interface = ["        "]
            interface_del = ["        "]

            for idx in range(0, len(entry["ns"]) - 1):
                if len(entry["ns"][idx]) == 0:
                    ns.append(self.cat_empty)
                    ns.append(", ")
                else:
                    ns.append("\n            |".join(entry["ns"][idx]))
                    if len(entry["ns"][idx]) > 1 and idx % 2 != 1:
                        ns.append(",\n        ")
                    else:
                        ns.append(",")

                interface.append("({}) {},".format(self.creation_interface, entry["interface"][idx]))
                interface.append("\n        ")

                interface_del.append("({}) {},".format(self.deletion_interface, entry["interface_del"][idx]))
                interface_del.append("\n        ")

                fixname[idx].append("/* {} */\n    ".format(entry["c_name"]))

                if entry["fixname"][idx] != "NULL":
                    fixname[idx].append("{{(const unsigned char *) \"{}\", {}}},".format(entry["fixname"][idx], len(entry["fixname"][idx])))
                else:
                    fixname[idx].append("{NULL, 0},")

                fixname[idx].append("\n    ")

                if idx % 2 == 1:
                    ns.append("\n        ")

            if len(entry["ns"][-1]) == 0:
                ns.append(self.cat_empty)
            else:
                ns.append("\n|".join(entry["ns"][-1]))

            interface.append("({}) {}".format(self.creation_interface, entry["interface"][-1]))
            interface_del.append("({}) {}".format(self.deletion_interface, entry["interface_del"][-1]))

            fixname[-1].append("/* {} */\n    ".format(entry["c_name"]))

            if entry["fixname"][-1] != "NULL":
                fixname[-1].append("{{(const unsigned char *) \"{}\", {}}},".format(entry["fixname"][-1], len(entry["fixname"][-1])))
            else:
                fixname[-1].append("{NULL, 0},")

            # res.append("{{(const unsigned char *) \"{}\", {}, {},\n        {{\n{}\n        }},\n        {{\n{}\n        }},\n        {{\n{}\n        }}\n    }}"
            #            .format(entry["name"], len(entry["name"]), entry["c_name"], "".join(ns), "".join(interface), "".join(fixname)))

            cats.append("/* {} */".format(entry["c_name"]), True)
            constructor.append("/* {} */".format(entry["c_name"]), True)
            destructor.append("/* {} */".format(entry["c_name"]), True)

            cats.append("{{\n{}\n    }}".format("".join(ns)))
            constructor.append("{{\n{}\n    }}".format("".join(interface)))
            destructor.append("{{\n{}\n    }}".format("".join(interface_del)))

        svg_fixname.append("{}".format("".join(fixname[4]))) # 4 == SVG namespace

        return [cats.create(1, False), self.interfaces.make_includes(),
                constructor.create(1, False),
                destructor.create(1, False),
                svg_fixname.create(1, False)]

    def ns_data_create(self):
        res = LXB.Res("pchtml_ns_data_t", self.ns_res_data, False, self.ns_last_entry_name)
        res_prefix = LXB.Res("pchtml_ns_prefix_data_t", self.ns_res_prefix_data, False, self.ns_last_entry_name)

        for name in self.ns_list:
            str_name = ''

            if len(self.ns[name]["link"]) > 16:
                str_name = ".u.long_str = (unsigned char *) \"{}\"".format(self.ns[name]["link"])
            else:
                str_name = ".u.short_str = \"{}\"".format(self.ns[name]["link"])

            res.append("{{{{{}, .length = {}, .next = NULL}}, {}, 1, true}}".format(
                str_name,
                len(self.ns[name]["link"]),
                self.ns[name]["c_name"]
            ))

            if len(self.ns[name]["name"]) > 16:
                str_name = ".u.long_str = (unsigned char *) \"{}\"".format(self.ns[name]["name"].lower())
            else:
                str_name = ".u.short_str = \"{}\"".format(self.ns[name]["name"].lower())

            res_prefix.append("{{{{{}, .length = {}, .next = NULL}}, {}, 1, true}}".format(
                str_name,
                len(self.ns[name]["name"]),
                self.ns[name]["c_name"]
            ))

        return [res.create(1, True), res_prefix.create(1, True)]

    def ns_make_hash(self, ns_list):
        result = []

        for name in ns_list:
            result.append(self.ns[name]["c_name"])

        return computeMD5hash("".join(result)).upper()

    def ns_hash_ifdef(self):
        result = []
        result.append("#ifdef PCHTML_NS_CONST_VERSION")
        result.append("#ifndef PCHTML_NS_CONST_VERSION_{}".format(self.ns_hash))
        result.append("#error Mismatched namespaces version! See \"lexbor/ns/const.h\".".format(self.ns_hash))
        result.append("#endif /* PCHTML_NS_CONST_VERSION_{} */".format(self.ns_hash))
        result.append("#else")
        result.append("#error You need to include \"lexbor/ns/const.h\".".format(self.ns_hash))
        result.append("#endif /* PCHTML_NS_CONST_VERSION */".format(self.ns_hash))

        return "\n".join(result)

    def ns_create(self):
        result = []

        frmt_ns = LXB.FormatEnum("{}_id_enum_t".format(self.ns_prefix))

        for idx in range(0, len(self.ns_list)):
            name = self.ns_list[idx]
            frmt_ns.append(self.ns[name]["c_name"], "0x{0:02x}".format(idx))

        frmt_ns.append(self.ns_last_entry_name, "0x{0:02x}".format(idx + 1))

        return frmt_ns.build()

    def ns_save(self, data, temp_file, save_to):
        lxb_temp = LXB.Temp(temp_file, save_to)
        lxb_temp.pattern_append("%%HASH%%", self.ns_hash)
        lxb_temp.pattern_append("%%BODY%%", "\n".join(data))

        lxb_temp.build()
        lxb_temp.save()

        if not WITHOUT_PRINT:
            print("\n".join(data))
            print("Save to {}".format(save_to))
            print("Done")

    def ns_create_and_save(self, temp_file, save_to):
        result = self.ns_create()
        self.ns_save(result, temp_file, save_to)

    def ns_shs_create(self, name):
        self.ns_shs = LXB.SHS(self.ns_shs_list, 0, False)

        test = self.ns_shs.make_test(5, 128)
        self.ns_shs.table_size_set(test[0][2])

        return self.ns_shs.create(name)

    def ns_shs_link_create(self, name):
        self.ns_shs_link = LXB.SHS(self.ns_shs_link_list, 0, False)

        test = self.ns_shs_link.make_test(5, 128)
        self.ns_shs_link.table_size_set(test[0][2])

        return self.ns_shs_link.create(name, 1)

    def ns_shs_save(self, data, data_link, temp_file, save_to):
        lxb_temp = LXB.Temp(temp_file, save_to)

        res, res_prefix = self.ns_data_create()

        lxb_temp.pattern_append("%%CHECK_NS_VERSION%%", self.ns_hash_ifdef())
        lxb_temp.pattern_append("%%NS_DATA%%", ''.join(res))
        lxb_temp.pattern_append("%%NS_PREFIX_DATA%%", ''.join(res_prefix))
        lxb_temp.pattern_append("%%SHS_DATA%%", ''.join(data))
        lxb_temp.pattern_append("%%SHS_DATA_LINK%%", ''.join(data_link))

        lxb_temp.build()
        lxb_temp.save()

        if not WITHOUT_PRINT:
            print("".join(data))
            print(self.shs_stat(self.ns_shs))
            print("Save to {}".format(save_to))
            print("Done")

    def ns_shs_create_and_save(self, name, name_link, temp_file, save_to):
        result = self.ns_shs_create(name)
        result_link = self.ns_shs_link_create(name_link)
        self.ns_shs_save(result, result_link, temp_file, save_to)

    def ns_test_name_create(self):
        result = []
        ns = self.ns

        for name in self.ns_list:
            if len(ns[name]["link"]) != 0:
                result.append("    entry = pchtml_ns_data_by_link(ns, (const unsigned char *) \"{}\", {});".format(ns[name]["link"], len(ns[name]["link"])))
                result.append("    test_ne(entry, NULL); test_eq_str(lexbor_hash_entry_str(&entry->entry), \"{}\");".format(ns[name]["link"]))

        return result

    def ns_test_create_and_save(self, temp_file, save_to):
        lxb_temp = LXB.Temp(temp_file, save_to)

        lxb_temp.pattern_append("%%TEST_NAMES%%", "\n".join(self.ns_test_name_create()))

        lxb_temp.build()
        lxb_temp.save()

        if not WITHOUT_PRINT:
            print("Test saved to {}".format(save_to))

    def enum_make_hash(self, enum_list):
        result = []

        for entry in enum_list:
            result.append(entry["name"])
            result.append(entry["c_name"])

            for ns_entry in entry["ns"]:
                result.append("".join(ns_entry))

        return computeMD5hash("".join(result)).upper()

    def enum_hash_ifdef(self):
        result = []
        result.append("#ifdef PCHTML_TAG_CONST_VERSION")
        result.append("#ifndef PCHTML_TAG_CONST_VERSION_{}".format(self.enum_hash))
        result.append("#error Mismatched tags version! See \"lexbor/tag/const.h\".".format(self.enum_hash))
        result.append("#endif /* PCHTML_TAG_CONST_VERSION_{} */".format(self.enum_hash))
        result.append("#else")
        result.append("#error You need to include \"lexbor/tag/const.h\".".format(self.enum_hash))
        result.append("#endif /* PCHTML_TAG_CONST_VERSION */".format(self.enum_hash))

        return "\n".join(result)

    def enum_create(self):
        result = []

        frmt_enum = LXB.FormatEnum("{}_id_enum_t".format(self.tag_prefix))

        for idx in range(0, len(self.enum_list)):
            frmt_enum.append(self.enum_list[idx]["c_name"], "0x{0:04x}".format(idx))

        frmt_enum.append(self.tag_last_entry_name, "0x{0:04x}".format(idx + 1))

        return frmt_enum.build()

    def enum_save(self, data, temp_file, save_to):
        lxb_temp = LXB.Temp(temp_file, save_to)
        lxb_temp.pattern_append("%%HASH%%", self.enum_hash)
        lxb_temp.pattern_append("%%BODY%%", "\n".join(data))

        lxb_temp.build()
        lxb_temp.save()

        if not WITHOUT_PRINT:
            print("\n".join(data))
            print("Save to {}".format(save_to))
            print("Done")

    def enum_create_and_save(self, temp_file, save_to):
        result = self.enum_create()
        self.enum_save(result, temp_file, save_to)

    def shs_create(self, name):
        self.shs = LXB.SHS(self.shs_list, 0, False)

        test = self.shs.make_test(128, 1024)
        self.shs.table_size_set(test[0][2])

        return self.shs.create(name)

    def shs_save(self, data, temp_file, save_to):
        res_lower, res_upper = self.tag_data_create_default()

        res_lower = ''.join(res_lower)
        res_upper = ''.join(res_upper)

        lxb_temp = LXB.Temp(temp_file, save_to)

        lxb_temp.pattern_append("%%CHECK_TAG_VERSION%%", self.enum_hash_ifdef())
        lxb_temp.pattern_append("%%TAG_DATA%%", res_lower)
        lxb_temp.pattern_append("%%TAG_DATA_UPPER%%", res_upper)
        lxb_temp.pattern_append("%%SHS_DATA%%", ''.join(data))

        lxb_temp.build()
        lxb_temp.save()

        if not WITHOUT_PRINT:
            print("".join(data))
            print(self.shs_stat(self.shs))
            print("Save to {}".format(save_to))
            print("Done")

    def shs_create_and_save_default(self, name, temp_file, save_to):
        result = self.shs_create(name)
        self.shs_save(result, temp_file, save_to)

    def shs_create_and_save_html(self, temp_file, save_to,
                                temp_file_interface, save_to_interface):
        data_list = self.tag_data_create_html()

        lxb_temp = LXB.Temp(temp_file, save_to)

        lxb_temp.pattern_append("%%CHECK_TAG_VERSION%%", self.enum_hash_ifdef())
        lxb_temp.pattern_append("%%CHECK_NS_VERSION%%", self.ns_hash_ifdef())
        lxb_temp.pattern_append("%%TAG_DATA%%", ''.join(data_list[0]))
        lxb_temp.pattern_append("%%FIXNAME_SVG_DATA%%", ''.join(data_list[4]))

        lxb_temp.build()
        lxb_temp.save()

        lxb_temp = LXB.Temp(temp_file_interface, save_to_interface)

        lxb_temp.pattern_append("%%CHECK_TAG_VERSION%%", self.enum_hash_ifdef())
        lxb_temp.pattern_append("%%CHECK_NS_VERSION%%", self.ns_hash_ifdef())
        lxb_temp.pattern_append("%%INCLUDES%%", '\n'.join(data_list[1]))
        lxb_temp.pattern_append("%%CONSTRUCTOR%%", ''.join(data_list[2]))
        lxb_temp.pattern_append("%%DESTRUCTOR%%", ''.join(data_list[3]))

        lxb_temp.build()
        lxb_temp.save()

        if not WITHOUT_PRINT:
            print(self.shs_stat(self.shs))
            print("Save to {}".format(save_to))
            print("Done")

    def tag_test_name_create(self):
        result = []

        for entry in self.shs_list:
            result.append("    entry = pchtml_tag_data_by_name(tags, (const unsigned char *) \"{}\", {});".format(entry["key"], len(entry["key"])))
            result.append("    test_ne(entry, NULL); test_eq_u_str(lexbor_hash_entry_str(&entry->entry), (const unsigned char *) \"{}\");".format(entry["key"]))

        return result

    def tag_test_create_and_save(self, temp_file, save_to):
        lxb_temp = LXB.Temp(temp_file, save_to)

        lxb_temp.pattern_append("%%TEST_NAMES%%", "\n".join(self.tag_test_name_create()))

        lxb_temp.build()
        lxb_temp.save()

        if not WITHOUT_PRINT:
            print("Test saved to {}".format(save_to))

    def shs_stat(self, shs):
       return "Max deep {}; Used {} of {}".format(shs.max, shs.used, shs.table_size)

if __name__ == "__main__":
    if len(sys.argv) > 2 and sys.argv[2] == '--without-print':
        WITHOUT_PRINT = 1

    if len(sys.argv) < 2:
        raise Exception('expecting target path')

    tags = Tags("tags.json", "interfaces.json")

    # print(tags.enum_hash_ifdef())
    tags.ns_create_and_save(
        "ns_const.h.in", "{}/ns_const.h".format(sys.argv[1]))
    tags.ns_shs_create_and_save(
        "pchtml_ns_res_shs_data", "pchtml_ns_res_shs_link_data",
        "ns_res.h.in", "{}/ns_res.h".format(sys.argv[1]))
    tags.enum_create_and_save(
        "tag_const.h.in", "{}/tag_const.h".format(sys.argv[1]))
    tags.shs_create_and_save_default(
        "pchtml_tag_res_shs_data_default",
        "tag_res.h.in", "{}/tag_res.h".format(sys.argv[1]))
    tags.shs_create_and_save_html(
        "html_tag_res.h.in", "{}/html_tag_res.h".format(sys.argv[1]),
        "html_interface_res.h.in", "{}/html_interface_res.h".format(sys.argv[1]))
    # tags.ns_test_create_and_save("tmp/test/ns_res.c", "../../../test/lexbor/ns/res.c")
    # tags.tag_test_create_and_save("tmp/test/tag_res.c", "../../../test/lexbor/tag/res.c")

