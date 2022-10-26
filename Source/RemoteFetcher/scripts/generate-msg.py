#!/usr/bin/env python
#
# Copyright (C) 2010 Apple Inc. All rights reserved.
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

from __future__ import with_statement
import sys
import re

import generator.msg

def load_msg_id():
    msg_ids = {}
    with open('MessageNames.h') as in_file:
        for line in in_file:
            line = line.strip()
            line = line.replace(', ', '')
            match = re.search(r'(?P<name>[A-Za-z_0-9]+) =\s*(?P<value>[0-9]+)', line)
            if match:
                name = match.group('name')
                value = match.group('value')
                msg_ids[name]=value
    return msg_ids

def main(argv):
    first_arg = True
    second_arg = False
    msg_ids = load_msg_id()
    for parameter in argv:
        if first_arg:
            first_arg = False
            second_arg = True
            continue
        if second_arg:
            base_dir = parameter
            second_arg = False
            continue

        receiver_name = parameter.rsplit('/', 1).pop()
        receiver_name = receiver_name.replace('_', '-')

        with open('%s/%s.msg.in' % (base_dir, parameter)) as source_file:
            receiver = generator.msg.parse(source_file, msg_ids)

        with open('fetcher-%s.h' % receiver_name, "w+") as header_output:
            header_output.write(generator.msg.gen_msg_header(receiver))

        with open('fetcher-%s.c' % receiver_name, "w+") as implementation_output:
            implementation_output.write(generator.msg.gen_msg_source(receiver))

    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv))
