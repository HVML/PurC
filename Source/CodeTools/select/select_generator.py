# This file is part of CSSEng.
# Licensed under the MIT License,
# http://www.opensource.org/licenses/mit-license.php
# Copyright 2017 Lucas Neves <lcneves@gmail.com>

import math
import string
import os
import sys
from select_config import values, groups
from assets import assets
from overrides import overrides

def get_tuple(from_var):
    """Convert tuples, strings and None into tuple."""
    if type(from_var) is tuple:
        return from_var
    elif type(from_var) is str:
        return (from_var,)
    elif from_var is None:
        return ()
    else:
        raise TypeError('Value should be either tuple, string or None, ' +
                        'received: ' + type(from_var).__name__)

def shift_star(value_type, prop_name):
    """Shift the asterisks from a pointer type to its name.

    Example: `lwc_string** str_array` would become
    `lwc_string **str_array`
    """
    star_i = value_type.find('*')
    v_type = value_type if star_i == -1 else value_type[:star_i]
    v_name = prop_name if star_i == -1 else value_type[star_i:] + prop_name
    return (v_type, v_name)

class Text:
    """Class for building strings for output files."""
    def __init__(self):
        self._lines = []
        self._comment = False
        self._esc_nl = False
        self._indent = 0

    name_width = 31
    bits_width = 16

    def indent(self, value):
        """Increase or decrease indent by <value>.

        Args:
            value <int>: positive or negative value to be added to
            indentation.
        """
        self._indent += value

    def comment(self):
        """Toggle C-style comment in the output text."""
        comm = self._comment
        self._comment = False
        self.append(' */' if comm else '/*')
        self._comment = not comm

    def escape_newline(self):
        """Toggle escape of newline character."""
        self._esc_nl = not self._esc_nl

    def append(self, text=None, pre_formatted=False):
        """Append text to file builder.

        Args:
            text:
                <str> add contents, breaking lines and adding comment
                    markers or newline escapes as needed.
                <list> recursively call this method for list items.
                <falsey value> add a new line.
            pre_formatted: just add text without preprocessing.
        """
        if not text:
            self._lines.append('{}{}{}'.format(
                '\t' * self._indent,
                ' * ' if self._comment else '',
                '\t' * (9 - self._indent) + '\\' if self._esc_nl else ''))
            return

        if isinstance(text, list):
            for t in text:
                self.append(t, pre_formatted)
            return

        if pre_formatted:
            self._lines.append(text)
            return

        line_break_before = [ c for c in ' +/' ]
        line_break_after = [ c for c in '({[' ]
        column_max = 72 if self._esc_nl else 80
        multiline = False

        while text:
            line = '\t' * self._indent
            if self._comment:
                line += ' * '
            prefix_size = (3 if self._comment else 0) + 8 * self._indent
            if prefix_size + len(text) <= column_max:
                line += text
                text = ''
            else:
                break_index = 0
                for c in (line_break_before + line_break_after):
                    after = 1 if c in line_break_after else 0
                    break_index = max(break_index,
                        text[:column_max - prefix_size].rfind(c) + after)
                break_index = break_index or len(text)
                line += text[:break_index].rstrip()
                text = text[break_index:].lstrip()
            if self._esc_nl:
                n_tabs = 9 - self._indent - math.floor(len(line.lstrip()) / 8)
                line += '\t' * n_tabs + '\\'
            self._lines.append(line)
            if text and not self._comment and not multiline:
                self.indent(2)
                multiline = True

        if multiline:
            self.indent(-2)

    def table_line(self):
        """Add a sum line for the tables in computed.h"""
        self.append('{0:{n}}{0:{b}}{0}'.format(
            '---', n=self.name_width, b=self.bits_width))

    def table_header(self):
        """Add a header line for the tables in computed.h"""
        self.append('{:{n}}{:{b}}{}'.format(
            'Property', 'Size (bits)', 'Size (bytes)', 
            n=self.name_width, b=self.bits_width))
        self.table_line()

    def result_line(self):
        """Add a result line for the tables in computed.h"""
        self.append(' ' * self.name_width + '=' * (self.bits_width + 3))

    def to_string(self):
        """Output contents of file builder as a string."""
        return '\n'.join(self._lines)

class CSSValue:
    """Values to be associated with properties.

    Args:
        name <str>: value name (required).
        css_type <str>: C type of value (required).
        size <int>: value size, in bytes (default: None, for pointers).
        defaults <str>: default value (default: 'NULL', for pointers).
        The fields below are only needed if the value stores data in the
        array of bits (currently only css_unit uses it).
        bits_name <str>: name of bits value (default: None).
        bits_type <str>: C type of bits value (default: None).
        bits_size <int>: value size, in bits (default: None).
        bits_defaults <str>: default value (default: '0').
    """
    def __init__(self, name, css_type, size=None, defaults='NULL',
                 bits_name=None, bits_type=None,
                 bits_size=None, bits_defaults='0'):
        self.name = name
        self.type = css_type
        self.size = size #  `None` means sizeof(ptr)
        self.defaults = defaults
        self.suffix = ''
        self.bits = None if bits_size is None else {
            'name': bits_name,
            'type': bits_type,
            'size': bits_size,
            'defaults': bits_defaults
        }

    @property
    def is_ptr(self):
        """Return True if value is a pointer; False otherwise."""
        return ((self.type, self.name) != shift_star(self.type, self.name))

class CSSProperty:
    """Class for CSS properties.

    Args:
        name <str>: property name (required).
        type_size <int>: opcode size, in bits (required).
        values <tuple or str>: property values (default: None).
          To set one value, using the value's defaults:
            'value_name'
          To set multiple values, using the values' defaults:
            (('value_name',), ('value_name',))
          To override the default of one or multiple values:
            (('value_name', 'default'),)
            (('value_name', 'default'), ('value_name', 'default'))
        condition <str>: condition (opcode value) to get property
        values in propget.h (default: None).
        defaults <str>: default opcode (default: None)
        comments <str>: comments for properties that are stored in
        "struct css_computed_{group}", instead of
        "struct css_computed_{group}_i (default: None)
        NOTE: passing this argument will result in the property being
        stored in "struct css_computed_{group}"!
        overrides <tuple | str>: files for which this property shouldn't
        autogenerate content; instead, read entry from from overrides.py
         Possible values:
           'get': overrides output to autogenerated_propget.h
           'set': overrides output to autogenerated_propset.h
           ('get', 'set'): overrides output to both files.
    """
    def __init__(self, name, type_size, values=None, condition=None,
             defaults=None, comments=None, override=None):
        self.name = name
        self.type_size = type_size
        self.values = self.make_values(values)
        self.defaults = defaults
        self.condition = condition
        self.override = get_tuple(override)
        self.comments = comments
        self.__mask = None
        self.index = None
        self.shift = None

    def make_values(self, vals):
        """Make list of values for this property."""
        if vals is None:
            return []
        elif type(vals) is str:
            return self.make_values(((vals,),))
        elif type(vals) is tuple:
            val_list = []
            for i, v in enumerate(vals):
                for x in values:
                    if x[0] == v[0]:
                        value = CSSValue(*x)
                        if len(v) == 2:
                            value.defaults = v[1]
                        if len(vals) > 1:
                            value.suffix = '_' + string.ascii_lowercase[i]
                        val_list.append(value)
                        break
                else:
                    raise ValueError('Value ' + v[0] + ' not found!')
            return val_list
        else:
            raise TypeError('Expected None, str or tuple, got ' +
                            type(vals).__name__)

    @property
    def bits_size(self):
        """Size of this property in the bits array."""
        return self.type_size + sum([ v.bits['size'] for v in self.values
            if v.bits is not None ])

    @property
    def bytes_size(self):
        """Size of this property's values, in bytes (excluding pointers)."""
        return sum([ v.size for v in self.values if v.size is not None ])

    @property
    def ptr_size(self):
        """Number of values of this property that are pointers."""
        return sum([ 1 for v in self.values if v.size is None ])

    @property
    def size_line(self):
        """String for computed.h with the sizes of this property."""
        name = '{:{width}}'.format(self.name, width=Text.name_width)
        type_size = '{:>3}'.format(str(self.type_size))
        extra_size = sum([ v.bits['size'] for v in self.values
                if v.bits is not None ])
        bits_size = '{:{width}}'.format(type_size +
                (' + ' + str(extra_size) if extra_size else ''),
                width=Text.bits_width)
        vars_size = '{:>3}'.format(
                str(self.bytes_size)) if self.bytes_size else ''
        ptr = ''
        for v in self.values:
            if v.size is None:
                ptr = 'sizeof(ptr)'
                break

        return (name + bits_size + vars_size +
                (' + ' if vars_size and ptr else '') + ptr)

    @property
    def mask(self):
        """Getter for the bitwise mask of this property in the bits array."""
        if self.__mask is None:
            raise NameError('Attribute `mask` not set yet!')
        return '0x{:x}'.format(self.__mask).lower()

    @mask.setter
    def mask(self, val):
        """Setter for the bitwise mask of this property in the bits array."""
        if type(val) is not int:
            raise TypeError('Value of `mask` must be an integer!')
        if val < 0:
            raise ValueError('Value of `mask` must be zero or positive!')
        self.__mask = val

    @property
    def def_undefs(self):
        """Return defines and undefs for propget.h and propset.h."""
        defines = [
            '#define {}_INDEX {}'.format(self.name.upper(), self.index),
            '#define {}_SHIFT {}'.format(self.name.upper(), self.shift),
            '#define {}_MASK {}'.format(self.name.upper(), self.mask)
        ]
        undefs = [
            '#undef {}_INDEX'.format(self.name.upper()),
            '#undef {}_SHIFT'.format(self.name.upper()),
            '#undef {}_MASK'.format(self.name.upper())
        ]
        return (defines, undefs)

    def get_param_values(self, pointer=False):
        """Make parameters for functions in propget.h and propset.h.

        Args:
            pointer <bool>: add a star before value name.
        """
        vals = []
        for v in self.values:
            vt, vn = shift_star(v.type, v.name)
            vn += v.suffix
            if pointer:
                vn = '*' + vn
                if v.name == 'counter_arr' or v.name == 'content_item':
                    vt = 'const ' + vt
            vals.append((vt, vn))
            if v.bits is not None:
                bt, bn = shift_star(v.bits['type'], v.bits['name'])
                bn += v.suffix
                if pointer:
                    bn = '*' + bn
                vals.append((bt, bn))
        return vals

    def get_bits(self):
        """Make vars for the bitwise operations in propget.h and propset.h."""
        bits = [
            { 'letter': v.suffix[1] if v.suffix else v.bits['name'][0],
              'name': v.bits['name'] + v.suffix,
              'size': v.bits['size'] }
            for v in self.values if v.bits is not None ]
        bits.append({ 'letter': 't', 'size': self.type_size, 'name': 'type' })
        bits_len = sum([ x['size'] for x in bits ])
        comment = '/* {}bit{}: {} : {} */'.format(
            bits_len,
            ('' if bits_len == 1 else 's'),
            ''.join([ b['letter'] * b['size'] for b in bits ]),
            ' | '.join([ b['name'] for b in bits ]))
        rev_bits = list(reversed(bits))
        type_mask = '0x{:x}'.format(
            sum([ 2 ** x for x in range(rev_bits[0]['size']) ])).lower()
        shift_list = [ (x['name'],
                        sum([ b['size'] for b in rev_bits[:(i + 1)] ]),
                        sum([ 2 ** x for x in range(x['size']) ]) * 2 **
                            sum([ b['size'] for b in rev_bits[:(i + 1)] ]))
            for i, x in enumerate(rev_bits[1:]) ]
        return (type_mask, shift_list, comment)

class Bin:
    """The storage unit for the bits array of properties."""
    def __init__(self, first_object):
        self.contents = [ first_object ]

    @property
    def size(self):
        return sum([ x.bits_size for x in self.contents ])

    def push(self, obj):
        self.contents.append(obj)

class CSSGroup:
    """Group of CSS properties (i.e. style, page, uncommon).

    Args:
        config <tuple>: imported from select_config.py.
    """
    def __init__(self, config):
        self.name = config['name']
        self.props = [ CSSProperty(*x) for x in config['props'] ]
        self.bits_array = self.make_bits_array()

    @property
    def bits_size(self):
        """Sum of all property bits in the bits array."""
        return sum([ p.bits_size for p in self.props ])

    @property
    def bytes_size(self):
        """Sum of all property value bytes (excluded pointers)."""
        return sum([ p.bytes_size for p in self.props ])

    @property
    def ptr_size(self):
        """Sum of all property pointers."""
        return sum([ p.ptr_size for p in self.props ])

    def make_bits_array(self):
        """Implement a `best fit first` heuristics for the bin packing
        of property bits in the bits array.
        Also generate index, shift and mask for each property in group."""

        bin_size = 32 # We're using uint32_t as concrete bins.
        bits_array = []
        props = sorted(self.props, key=(lambda x: x.bits_size), reverse=True)

        for p in props:
            for b in bits_array:
                if b.size + p.bits_size <= bin_size:
                    b.push(p)
                    p.shift = (bin_size -
                               sum([ x.bits_size for x in b.contents ]))
                    break
            else:
                p.shift = bin_size - p.bits_size
                bits_array.append(Bin(p))

            p.mask = (sum([ 2 ** x for x in range(p.bits_size) ]) *
                      2 ** p.shift)
            bits_array.sort(key=(lambda x: x.size), reverse=True)

        for i, b in enumerate(bits_array):
            for p in b.contents:
                p.index = i

        return bits_array

    def get_idot_grp(self):
        """Make parameters for accessing bits and values in this group."""
        i_dot = '' if self.name == 'page' else 'i.'
        grp = '' if self.name == 'style' else '->{}{}'.format(
            '' if self.name == 'page' else i_dot, self.name)
        return (i_dot, grp)

    def make_computed_h(self):
        """Output this group's text for the computed.h file."""
        t = Text()
        t.append()

        typedef = 'typedef ' if self.name == 'page' else ''
        t.append('{}struct css_computed_{}{} {{'.format(
            typedef, self.name, '' if self.name == 'page' else '_i'))

        t.comment()
        commented = []
        t.table_header()

        for prop in sorted(self.props, key=(lambda x: x.name)):
            if prop.comments is None:
                t.append(prop.size_line)
            else:
                commented.extend(( '', prop.comments, '', prop.size_line ))

        t.append(commented)
        t.append()
        t.table_line()
        t.append('{:{len_1}}{:>3}{:{len_2}}{:>3}{}{}'.format('',
            str(self.bits_size), ' bits', str(self.bytes_size),
            ' + ' + str(self.ptr_size) + 'sizeof(ptr)'
                    if self.ptr_size else '',
            ' bytes',
            len_1=Text.name_width, len_2=(Text.bits_width - 3)))
        t.result_line()
        t.append('{:{len_1}}{:>3}{}{}'.format('',
            math.ceil(self.bits_size / 8) + self.bytes_size,
            ' + ' + str(self.ptr_size) + 'sizeof(ptr)'
                    if self.ptr_size else '',
            ' bytes', len_1=Text.name_width))
        t.append()

        t.append('Bit allocations:')
        for i, b in enumerate(self.bits_array):
            bits = []
            for prop in b.contents:
                for char in prop.name + prop.name.upper():
                    if char not in bits and char in string.ascii_letters:
                        bits.extend(char * prop.bits_size)
                        break
            t.append()
            t.append('{:<2} {:.<32}'.format(str(i), ''.join(bits)))
            t.append('; '.join([ p.name for p in b.contents ]))
        t.comment()

        t.indent(1)
        t.append('uint32_t bits[' + str(len(self.bits_array)) + '];')
        t.append()
        t.append(self.make_value_declaration(for_commented=False))

        if self.name == 'style':
            t.append()
            for g in css_groups:
                if g.name != 'style' and g.name != 'page':
                    t.append('css_computed_{0} *{0};'.format(g.name))

        t.append('uint32_t grid_template_columns_size;')
        t.append('uint8_t grid_template_columns_type;')
        t.append('css_fixed* grid_template_columns;')
        t.append('css_unit*  grid_template_columns_unit;')

        t.append();
        t.append('uint32_t grid_template_rows_size;')
        t.append('uint8_t grid_template_rows_type;')
        t.append('css_fixed* grid_template_rows;')
        t.append('css_unit*  grid_template_rows_unit;')

        t.append();
        t.append('css_fixed  text_shadow_h;')
        t.append('css_unit  text_shadow_h_unit;')
        t.append('css_fixed  text_shadow_v;')
        t.append('css_unit  text_shadow_v_unit;')
        t.append('css_fixed  text_shadow_blur;')
        t.append('css_unit  text_shadow_blur_unit;')
        t.append('css_color  text_shadow_color;')

        t.append();
        t.append('uint32_t stroke_dasharray_size;')
        t.append('css_fixed* stroke_dasharray;')
        t.append('css_unit*  stroke_dasharray_unit;')

        t.append();
        t.append('lwc_string* fill;')
        t.append('css_color fill_color;')
        t.append('lwc_string* stroke;')
        t.append('css_color stroke_color;')

        t.indent(-1)
        t.append('}}{};'.format(
            ' css_computed_' + self.name if typedef else ''))

        if self.name != 'page':
            typedef = 'typedef ' if self.name != 'style' else ''
            t.append()
            t.append('{}struct css_computed_{} {{'.format(
                     typedef, self.name))
            t.indent(1)
            t.append('struct css_computed_' + self.name + '_i i;')
            t.append()
            t.append(self.make_value_declaration(for_commented=True))
            t.append()

            t.append('struct css_computed_' + self.name + ' *next;')
            t.append('uint32_t count;')
            t.append('uint32_t bin;')
            t.indent(-1)
            t.append('}}{};'.format(
                ' css_computed_' + self.name if typedef else ''))

        return t.to_string()

    def make_propset_h(self):
        """Output this group's property functions for the propset.h file.

        If group is not `style`, will also output the defaults
        and the ENSURE_{group} texts.
        """
        t = Text()
        i_dot, grp = self.get_idot_grp()

        if self.name != 'style':
            t.append('static const css_computed_{0} default_{0} = {{'.format(
                self.name))
            t.indent(1)

            if self.name != 'page':
                t.append('.i = {')
                t.indent(1)

            t.append('.bits = {') 
            t.indent(1)

            bits_ops = []
            for b in self.bits_array:
                or_ops = []
                for p in b.contents:
                    or_ops.append('({} << {})'.format(p.defaults, str(p.shift))
                                  if p.shift else p.defaults)
                bits_ops.append(' | '.join(or_ops))

            t.append(',\n'.join(bits_ops).split('\n'))
            t.indent(-1)
            t.append('},')
            t.append(',\n'.join(
                self.make_value_declaration(False, True)).split('\n'))

            if self.name != 'page':
                t.indent(-1)
                t.append('},')
                t.append(',\n'.join(
                    self.make_value_declaration(True, True) +
                    [ '.next = NULL', '.count = 0', '.bin = UINT32_MAX' ]
                    ).split('\n'))

            t.indent(-1)
            t.append('};')

            t.append()
            t.escape_newline()
            t.append('#define ENSURE_{} do {{'.format(self.name.upper()))
            t.indent(1)
            t.append('if (style->{}{} == NULL) {{'.format(i_dot, self.name))
            t.indent(1)
            t.append('style->{}{n} = malloc(sizeof(css_computed_{n}));'.format(
                i_dot, n=self.name))
            t.append('if (style->{}{} == NULL)'.format(i_dot, self.name))
            t.indent(1)
            t.append('return CSS_NOMEM;')
            t.indent(-1)
            t.append()
            t.append('memcpy(style->{}{n}, &default_{n}, '
                     'sizeof(css_computed_{n}));'.format(i_dot, n=self.name))
            t.indent(-1)
            t.append('}')
            t.indent(-1)
            t.append('} while(0)')
            t.escape_newline()
            t.append()

        for p in sorted(self.props, key=(lambda x: x.name)):
            defines, undefs = p.def_undefs

            t.append()
            t.append(defines)

            if p.name in overrides['set']:
                t.append(overrides['set'][p.name], pre_formatted=True)
                t.append(undefs)
                continue

            vals = p.get_param_values()
            params = ', '.join([ 'css_computed_style *style', 'uint8_t type' ]
                               + [ ' '.join(x) for x in vals ])
            t.append()
            t.append('static inline css_error set_{}({})'.format(
                p.name, params))
            t.append('{')
            t.indent(1)

            t.append('uint32_t *bits;')
            t.append()

            if self.name != 'style':
                t.append('ENSURE_{};'.format(self.name.upper()))
                t.append()

            t.append('bits = &style{}->{}bits[{}_INDEX];'.format(
                grp, i_dot, p.name.upper()))
            t.append()

            type_mask, shift_list, bits_comment = p.get_bits()
            t.append(bits_comment)
            type_mask = '((uint32_t)type & {})'.format(type_mask)
            val_list = [ '({} << {})'.format(x[0], x[1]) for x in shift_list ]
            ops_str = ' | '.join([ type_mask ] + val_list)
            t.append('*bits = (*bits & ~{0}_MASK) | '
                     '({1}{2}{3} << {0}_SHIFT);'.format(
                         p.name.upper(),
                         '(' if val_list else '',
                         ops_str,
                         ')' if val_list else ''))

            t.append()
            for v in p.values:
                old_n = 'old_' + v.name + v.suffix
                old_t, old_n_shift = shift_star(v.type, old_n)

                if v.name == 'string':
                    t.append('{} {} = style{}->{}{};'.format(
                        old_t, old_n_shift,
                        grp, i_dot, p.name + v.suffix))
                    t.append()
                    t.append('if ({} != NULL) {{'.format(v.name + v.suffix))
                    t.indent(1)
                    t.append('style{}->{}{} = lwc_string_ref({});'.format(
                        grp, i_dot, p.name + v.suffix, v.name + v.suffix))
                    t.indent(-1)
                    t.append('} else {')
                    t.indent(1)
                    t.append('style{}->{}{} = NULL;'.format(
                        grp, i_dot, p.name + v.suffix))
                    t.indent(-1)
                    t.append('}')
                    t.append()
                    t.append('if ({} != NULL)'.format(old_n))
                    t.indent(1)
                    t.append('lwc_string_unref({});'.format(old_n))
                    t.indent(-1)

                elif v.name == 'string_arr' or v.name == 'counter_arr':
                    iter_var = 's' if v.name == 'string_arr' else 'c'
                    iter_deref = '*s' if v.name == 'string_arr' else 'c->name'
                    t.append('{} {} = style{}->{};'.format(
                        old_t, old_n_shift,
                        grp, p.name + v.suffix))
                    t.append('{} {};'.format(old_t,
                                             shift_star(v.type, iter_var)[1]))
                    t.append()
                    t.append('for ({0} = {2}; {0} != NULL && '
                             '{1} != NULL; {0}++)'.format(iter_var, iter_deref,
                                                          v.name + v.suffix))
                    t.indent(1)
                    t.append('{0} = lwc_string_ref({0});'.format(iter_deref))
                    t.indent(-1)
                    t.append()
                    t.append('style{}->{} = {};'.format(
                        grp, p.name + v.suffix, v.name + v.suffix))
                    t.append()
                    t.append('/* Free existing array */')
                    t.append('if ({} != NULL) {{'.format(old_n))
                    t.indent(1)
                    t.append('for ({0} = {2}; {1} != NULL; {0}++)'.format(
                        iter_var, iter_deref, old_n))
                    t.indent(1)
                    t.append('lwc_string_unref({});'.format(iter_deref))
                    t.indent(-1)
                    t.append()
                    t.append('if ({} != {})'.format(old_n, v.name + v.suffix))
                    t.indent(1)
                    t.append('free({});'.format(old_n))
                    t.indent(-2)
                    t.append('}')

                elif not v.is_ptr:
                    t.append('style{}->{}{} = {};'.format(
                        grp, i_dot, p.name + v.suffix, v.name + v.suffix))

                else:
                    raise ValueError('Cannot handle value ' + v.name +'!')

                t.append()
            t.append('return CSS_OK;')
            t.indent(-1)
            t.append('}')
            t.append(undefs)

        return t.to_string()

    def make_propget_h(self):
        """Output this group's property functions for the propget.h file."""
        t = Text()
        i_dot, grp = self.get_idot_grp()

        for p in sorted(self.props, key=(lambda x: x.name)):
            defines, undefs = p.def_undefs

            t.append()
            t.append(defines)

            if p.name in overrides['get']:
                t.append(overrides['get'][p.name], pre_formatted=True)
                t.append(undefs)
                continue

            vals = p.get_param_values(pointer=True)
            params = ', '.join([ 'css_computed_style *style' ]
                               + [ ' '.join(x) for x in vals ])
            t.append('static inline uint8_t get_{}(const {})'.format(
                p.name, params))
            t.append('{')
            t.indent(1)

            if self.name != 'style':
                t.append('if (style{} != NULL) {{'.format(grp))
                t.indent(1)

            t.append('uint32_t bits = style{}->{}bits[{}_INDEX];'.format(
                grp, i_dot, p.name.upper()))
            t.append('bits &= {}_MASK;'.format(p.name.upper()))
            t.append('bits >>= {}_SHIFT;'.format(p.name.upper()))
            t.append()

            type_mask, shift_list, bits_comment = p.get_bits()
            t.append(bits_comment)

            if p.condition:
                t.append('if ((bits & {}) == {}) {{'.format(
                    type_mask, p.condition))
                t.indent(1)

            for v in p.values:
                this_idot = '' if v.is_ptr and v.name != 'string' else i_dot
                t.append('*{} = style{}->{}{};'.format(
                    v.name + v.suffix, grp, this_idot, p.name + v.suffix))
            for i, v in enumerate(list(reversed(shift_list))):
                if i == 0:
                    t.append('*{} = bits >> {};'.format(v[0], v[1]))
                else:
                    t.append('*{} = (bits & 0x{:x}) >> {};'.format(
                        v[0], v[2], v[1]).lower())

            if p.condition:
                t.indent(-1)
                t.append('}')

            t.append()
            t.append('return (bits & {});'.format(type_mask))

            if self.name != 'style':
                t.indent(-1)
                t.append('}')
                t.append()
                t.append('/* Initial value */')
                for v in p.values:
                    t.append('*{} = {};'.format(v.name + v.suffix, v.defaults))
                    if v.bits is not None:
                        t.append('*{} = {};'.format(
                            v.bits['name'] + v.suffix, v.bits['defaults']))
                t.append('return {};'.format(p.defaults))

            t.indent(-1)
            t.append('}')
            t.append(undefs)

        return t.to_string()

    def make_value_declaration(self, for_commented, defaults=False):
        """Output declarations of values for this group's properties.

        Args:
            for_commented: only parse values that have a `comment` field
            defaults: outputs default value assignments.
        """

        r = []
        for p in sorted(self.props, key=(lambda x: x.name)):
            if bool(p.comments) == for_commented:
                for v in p.values:
                    if defaults:
                        r.append('.{}{} = {}'.format(p.name, v.suffix,
                                                     v.defaults))
                    else:
                        v_type, v_name = shift_star(v.type, p.name)
                        r.append('{} {}{};'.format(v_type, v_name, v.suffix))
        return r

    def make_text(self, filename):
        """Return this group's text for the given file."""
        if filename == 'computed.h':
            return self.make_computed_h()
        elif filename == 'propset.h':
            return self.make_propset_h()
        elif filename == 'propget.h':
            return self.make_propget_h()
        else:
            raise ValueError()

css_groups = [ CSSGroup(g) for g in groups ]
if sys.argv[1] is not None:
    dir_path = sys.argv[1]
    os.makedirs(dir_path, exist_ok = True)
else:
    dir_path = os.path.dirname(os.path.realpath(__file__))

for k, v in assets.items():
    # Key is filename string (e.g. "computed.h") without autogenerated_ prefix
    body = '\n'.join([ g.make_text(k) for g in css_groups ])
    text = '\n'.join([ v['header'], body, v['footer'] ])
    with open(os.path.join(dir_path, 'autogenerated_') + k, 'w') as file_k:
        file_k.write(text)
