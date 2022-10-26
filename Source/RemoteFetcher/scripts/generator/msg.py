# Copyright (C) 2010-2017 Apple Inc. All rights reserved.
# Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import re

from generator import model

g_inner_type = ['pcfetcher_string', 'pcfetcher_data_reference']

def bracket_if_needed(condition):
    if re.match(r'.*(&&|\|\|).*', condition):
        return '(%s)' % condition
    else:
        return condition

def parse(file, msg_ids):
    receiver_attributes = None
    destination = None
    name = None
    messages = []
    conditions = False
    master_condition = None
    superclass = []
    msg_id = -1
    for line in file:
        line = line.strip()
        match = re.search(r'ID (?P<name>[A-Za-z_0-9]+)', line)
        if match:
            msg_id = msg_ids[match.group('name')]
        match = re.search(r'msg -> (?P<name>[A-Za-z_\-0-9]+) \s*(?::\s*(?P<superclass>.*?) \s*)?(?:(?P<attributes>.*?)\s+)?{', line)
        if match:
            receiver_attributes = parse_attributes_string(match.group('attributes'))
            if match.group('superclass'):
                superclass = match.group('superclass')
            name = 'msg_' + match.group('name')
            name = name.replace('-', '_')
            conditions = True
            continue
        match = re.search(r'base -> (?P<name>[A-Za-z_\-0-9]+) \s*(?::\s*(?P<superclass>.*?) \s*)?(?:(?P<attributes>.*?)\s+)?{', line)
        if match:
            receiver_attributes = parse_attributes_string(match.group('attributes'))
            if match.group('superclass'):
                superclass = match.group('superclass')
            name = match.group('name')
            name = name.replace('-', '_')
            conditions = False
            continue
        match = re.search(r'(.*);', line)
        if match:
            parameters_string = match.groups()
            if parameters_string:
                parameters = parse_parameters_string(parameters_string)
            else:
                parameters = []

            messages.append(model.Message(name, parameters, None, None, conditions))
    return model.MessageReceiver(name, superclass, receiver_attributes, messages, conditions, msg_id)


def parse_attributes_string(attributes_string):
    if not attributes_string:
        return None
    return attributes_string.split()


def split_parameters_string(parameters_string):
    parameters = []
    current_parameter_string = ''

    nest_level = 0
    for character in parameters_string:
        if character == ',' and nest_level == 0:
            parameters.append(current_parameter_string)
            current_parameter_string = ''
            continue

        if character == '<':
            nest_level += 1
        elif character == '>':
            nest_level -= 1

        current_parameter_string += character

    parameters.append(current_parameter_string)
    return parameters


def parse_parameters_string(parameters_string):
    parameters = []

    for parameter_string in split_parameters_string(parameters_string):
        match = re.search(r'\s*(?:\[(?P<attributes>.*?)\]\s+)?(?P<type_and_name>.*)', parameter_string)
        attributes_string, type_and_name_string = match.group('attributes', 'type_and_name')

        split = type_and_name_string.rsplit(' ', 1)
        parameter_kind = 'base'
        if split[0].startswith('array '):
            parameter_kind = 'array'
            split[0] = split[0][6:]
        elif split[0].startswith('array_4 '):
            parameter_kind = 'array_4'
            split[0] = split[0][8:]
        elif split[0].startswith('struct '):
            parameter_kind = 'struct'
            split[0] = split[0][7:]
        elif split[0].startswith('enum'):
            parameter_kind = split[0][:split[0].find(' ')]
            split[0] = split[0][split[0].find(' ') + 1:]

        parameter_type = split[0]
        parameter_name = split[1]

        parameters.append(model.Parameter(kind=parameter_kind, type=parameter_type, name=parameter_name, attributes=parse_attributes_string(attributes_string)))
    return parameters

def gen_msg_header(receiver):
    result = []

    result.append('#ifndef PURC_FETCHER_%s_H\n' % receiver.name.upper())
    result.append('#define PURC_FETCHER_%s_H\n' % receiver.name.upper())
    result.append('\n')

    result.append('#include "fetcher-msg.h"\n')

    for parameter in receiver.iterparameters():
        kind = parameter.kind
        type = parameter.type
        if type in g_inner_type:
            continue

        if kind != 'base':
            result.append('#include "%s.h"\n' % type.replace('_', '-').replace('pcfetcher', 'fetcher'))

    result.append('\n')
    result.append('struct pcfetcher_%s {\n' % receiver.name)
    if receiver.condition:
        result.append('    struct pcfetcher_msg_header header;\n')
    result.append('\n')

    for parameter in receiver.iterparameters():
        kind = parameter.kind
        type = parameter.type
        name = parameter.name
        if kind == 'base':
            result.append('    %s %s;\n' % (type, name))
        elif kind == 'struct':
            result.append('    struct %s* %s;\n' % (type, name))
        elif kind == 'enum':
            result.append('    enum %s %s;\n' % (type, name))
        elif kind == 'array':
            result.append('\n')
            result.append('    // %s array\n' % type)
            result.append('    struct pcutils_arrlist* %s;\n' % name)
        elif kind == 'array_4':
            result.append('\n')
            result.append('    // %s array, header 4 bytes\n' % type)
            result.append('    struct pcutils_arrlist* %s;\n' % name)

    result.append('};\n\n')

    result.append('struct pcfetcher_%s* pcfetcher_%s_create(void);\n\n' % (receiver.name, receiver.name))
    result.append('void pcfetcher_%s_destroy(struct pcfetcher_%s*);\n\n' % (receiver.name, receiver.name))
    result.append('void pcfetcher_%s_encode(struct pcfetcher_encoder*, void*);\n\n' % receiver.name)
    result.append('bool pcfetcher_%s_decode(struct pcfetcher_decoder*, void**);\n\n' % receiver.name)

    result.append('\n')
    result.append('static inline void pcfetcher_%s_array_free_fn(void* v)\n' % receiver.name)
    result.append('{\n')
    result.append('    pcfetcher_%s_destroy((struct pcfetcher_%s*)v);\n' % (receiver.name, receiver.name))
    result.append('}\n')

    result.append('\n')
    result.append('static inline struct pcutils_arrlist* pcfetcher_%s_array_create(void)\n' % receiver.name)
    result.append('{\n')
    result.append('    return pcfetcher_array_create(pcfetcher_%s_array_free_fn);\n' % receiver.name)
    result.append('}\n')

    result.append('\n')
    result.append('static inline void pcfetcher_%s_array_destroy(struct pcutils_arrlist* array)\n' % receiver.name)
    result.append('{\n')
    result.append('    pcfetcher_array_destroy(array);\n')
    result.append('}\n')

    result.append('\n')
    result.append('static inline void pcfetcher_%s_array_encode(\n' % receiver.name)
    result.append('        struct pcfetcher_encoder* encoder, struct pcutils_arrlist* array, uint8_t header_bytes)\n')
    result.append('{\n')
    result.append('    pcfetcher_array_encode(encoder, array, pcfetcher_%s_encode, header_bytes);\n' % receiver.name)
    result.append('}\n')

    result.append('\n')
    result.append('static inline void pcfetcher_%s_array_decode(\n' % receiver.name)
    result.append('        struct pcfetcher_decoder* decoder, struct pcutils_arrlist** array, uint8_t header_bytes)\n')
    result.append('{\n')
    result.append('    pcfetcher_array_decode(decoder, array,\n')
    result.append('         pcfetcher_%s_array_create,\n' % receiver.name)
    result.append('         pcfetcher_%s_decode, header_bytes);\n' % receiver.name)
    result.append('}\n')

    if receiver.msg_id > 0:
        result.append('\n')
        result.append('static inline uint32_t pcfetcher_%s_id(void)\n' % (receiver.name))
        result.append('{\n')
        result.append('    return %s;\n' % receiver.msg_id)
        result.append('}\n')

    result.append('\n')
    result.append('#endif /* PURC_FETCHER_%s_H */\n' % receiver.name.upper())
    result.append('\n')

    return ''.join(result)

def gen_msg_source(receiver):
    result = []
    msg_name = receiver.name

    result.append('#include "fetcher-%s.h"\n' % msg_name.replace('_', '-'))
    result.append('\n')
    result.append('#include <stdlib.h>\n')
    result.append('\n')

    result.append('struct pcfetcher_%s* pcfetcher_%s_create(void)\n' % (msg_name, msg_name))
    result.append('{\n')
    result.append('    return (struct pcfetcher_%s*)\n' % (msg_name));
    result.append('            calloc(sizeof(struct pcfetcher_%s), 1);\n' % msg_name);
    result.append('}\n\n')

    result.append('void pcfetcher_%s_destroy(struct pcfetcher_%s* msg)\n' % (msg_name, msg_name))
    result.append('{\n')
    result.append('    if (!msg) {\n');
    result.append('        return;\n');
    result.append('    }\n');
    for parameter in receiver.iterparameters():
        kind = parameter.kind
        type = parameter.type
        name = parameter.name
        if kind == 'struct':
            result.append('    %s_destroy(msg->%s);\n' % (type, name))
        elif kind == 'array':
            result.append('    %s_array_destroy(msg->%s);\n' % (type, name))
        elif kind == 'array_4':
            result.append('    %s_array_destroy(msg->%s);\n' % (type, name))
    result.append('}\n\n')

    result.append('void pcfetcher_%s_encode(struct pcfetcher_encoder* encoder, void* v)\n' % msg_name)
    result.append('{\n')
    result.append('    struct pcfetcher_%s* msg = (struct pcfetcher_%s*)v;\n' % (msg_name, msg_name))

    if receiver.condition:
        result.append('    pcfetcher_msg_header_encode(encoder, &msg->header);\n')

    for parameter in receiver.iterparameters():
        kind = parameter.kind
        type = parameter.type
        name = parameter.name
        if kind == 'base':
            result.append('    pcfetcher_basic_encode(encoder, msg->%s);\n' % name)
        elif kind == 'enum':
            result.append('    pcfetcher_basic_encode(encoder, msg->%s);\n' % name)
        elif kind == 'struct':
            result.append('    %s_encode(encoder, msg->%s);\n' % (type, name))
        elif kind == 'array':
            result.append('    %s_array_encode(encoder, msg->%s, 8);\n' % (type, name))
        elif kind == 'array_4':
            result.append('    %s_array_encode(encoder, msg->%s, 4);\n' % (type, name))

    result.append('}\n\n')

    result.append('bool pcfetcher_%s_decode(struct pcfetcher_decoder* decoder, void** v)\n' % msg_name)
    result.append('{\n')
    result.append('    struct pcfetcher_%s* msg = pcfetcher_%s_create();\n' % (msg_name, msg_name));

    if receiver.condition:
        result.append('    pcfetcher_msg_header_decode(decoder, &msg->header);\n')

    for parameter in receiver.iterparameters():
        kind = parameter.kind
        type = parameter.type
        name = parameter.name
        if kind == 'base':
            result.append('    pcfetcher_basic_decode(decoder, msg->%s);\n' % name)
        elif kind == 'enum':
            result.append('    pcfetcher_basic_decode(decoder, msg->%s);\n' % name)
        elif kind == 'struct':
            result.append('    %s_decode(decoder, (void**)&msg->%s);\n' % (type, name))
        elif kind == 'array':
            result.append('    %s_array_decode(decoder, &msg->%s, 8);\n' % (type, name))
        elif kind == 'array_4':
            result.append('    %s_array_decode(decoder, &msg->%s, 4);\n' % (type, name))

    result.append('\n')
    result.append('    *v = msg;\n')
    result.append('    return true;\n')
    result.append('}\n\n')

    return ''.join(result)

