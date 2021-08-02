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

attributes_name = [
    "id", "class", "dir", "width", "height", "pool", "href", "alt", "disabled", "src",
    "style", "title", "checked", "maxlength", "content", "http-equiv", "scheme",
    "charser", "is", "for", "slot", "html", "system", "public", "type", "color",
    "face", "size"
]


attributes_name.sort()
attributes_name.insert(0, "_undef")

WITHOUT_PRINT = 0

class Attr:
    prefix = "PCEDOM_ATTR_"
    shs_name = "pcedom_attr_res_shs_data"
    data_name = "pcedom_attr_res_data_default"

    def __init__(self):
        pass

    def make_enum_name(self, name):
        name = re.sub(r"[^a-zA-Z0-9_]", "_", name)
        return "{}{}".format(self.prefix, name.upper())

    def data_create(self):
        res = LXB.Res("pcedom_attr_data_t", self.data_name, False, self.make_enum_name("_LAST_ENTRY"))

        for name in attributes_name:
            variable = ".u.short_str = \"{}\"".format(name)

            if len(name) > 16:
                variable = ".u.long_str = (lxb_char_t *) \"{}\"".format(name)

            if name == '_undef':
                res.append("{{{{.u.short_str = \"{}\", .length = {}, .next = NULL}},\n     {}, 1, true}}".format("#undef", len("#undef"), self.make_enum_name("_undef") ))
            else:
                res.append("{{{{{}, .length = {}, .next = NULL}},\n     {}, 1, true}}".format(variable, len(name), self.make_enum_name(name)))

        return res.create(rate = 1, is_const = True)

    def shs_create(self):
        attrs = []

        for name in attributes_name:
            if name == '_undef':
                var = "#undef"
            else:
                var = name

            attrs.append({"key": var,
                          "value": "(void *) &{}[{}]".format(self.data_name, 
                                                    self.make_enum_name(name))})

        shs = LXB.SHS(attrs, 0, False)

        test = shs.make_test(5, 128)
        shs.table_size_set(test[0][2])

        res = shs.create(self.shs_name, rate = 1)

        if not WITHOUT_PRINT:
            print(self.shs_stat(shs))

        return res

    def const_create(self):
        frmt_enum = LXB.FormatEnum("{}id_enum_t".format(self.prefix.lower()))

        for idx in range(0, len(attributes_name)):
            frmt_enum.append(self.make_enum_name(attributes_name[idx]), "0x{0:04x}".format(idx))

        frmt_enum.append(self.make_enum_name("_LAST_ENTRY"), "0x{0:04x}".format(idx + 1))

        return frmt_enum.build()

    def shs_stat(self, shs):
       return "Max deep {}; Used {} of {}".format(shs.max, shs.used, shs.table_size)

    def shs_save(self, temp_file, save_to):
        res = self.shs_create()
        data = self.data_create()

        lxb_temp = LXB.Temp(temp_file, save_to)

        lxb_temp.pattern_append("%%SHS_DATA%%", ''.join(data))
        lxb_temp.pattern_append("%%SHS%%", ''.join(res))

        lxb_temp.build()
        lxb_temp.save()

        if not WITHOUT_PRINT:
            print("".join(data))
            print("".join(res))
            print("Save to {}".format(save_to))
            print("Done")

    def const_save(self, temp_file, save_to):
        res = self.const_create()

        lxb_temp = LXB.Temp(temp_file, save_to)

        lxb_temp.pattern_append("%%BODY%%", '\n'.join(res))

        lxb_temp.build()
        lxb_temp.save()

        if not WITHOUT_PRINT:
            print("\n".join(res))
            print("Save to {}".format(save_to))
            print("Done")

if __name__ == "__main__":
    if len(sys.argv) > 2 and sys.argv[2] == '--without-print':
        WITHOUT_PRINT = 1

    if len(sys.argv) < 2:
        raise Exception('expecting target path')

    attr = Attr()

    attr.const_save("attr_const.h.in", "{}/html_attr_const.h".format(sys.argv[1]))
    attr.shs_save("attr_res.h.in", "{}/html_attr_res.h".format(sys.argv[1]))

