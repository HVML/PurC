# PurC

`PurC` is the Prime hVml inteRpreter for C language. It is also
the abbreviation of `Purring Cat`, while Purring Cat is the nickname
and the mascot of HVML.

**Table of Contents**

[//]:# (START OF TOC)

- [Introduction](#introduction)
- [Source Tree of PurC](#source-tree-of-purc)
- [Current Status](#current-status)
- [Building](#building)
   + [Commands](#commands)
   + [Using the test samples](#using-the-test-samples)
   + [Other documents](#other-documents)
- [Authors and Contributors](#authors-and-contributors)
- [Copying](#copying)
- [Tradmarks](#tradmarks)

[//]:# (END OF TOC)

## Introduction

`PurC` is the Prime HVML inteRpreter for C language. It is also
the abbreviation of `Purring Cat`, while Purring Cat is the nickname
and the mascot of HVML, which is a new-style programming language proposed
by [Vincent Wei].

For more information about HVML, please refer to the article
_HVML, a Programable Markup Language_:

- [Link on GitHub](https://github.com/HVML/hvml-docs/blob/master/en/an-introduction-to-hvml-en.md)
- [Link on GitLab](https://gitlab.fmsoft.cn/hvml/hvml-docs/-/blob/master/en/an-introduction-to-hvml-en.md)

PurC implements all features defined by [HVML Specifiction V1.0] in C language.
PurC also implements all predefined dynamic variables defined by
[HVML Predefined Variables V1.0].

PurC provides support for Linux and macOS. The support for Windows is
on the way. We welcome others to port PurC to other platforms.

You can use PurC to run a HVML program by using the command line tool `purc`, or
use PurC as a library to build your own HVML interpreter. We release PurC under
LGPLv3, so it is free for commercial use if you follow the conditions and terms
of LGPLv3.

For documents or other open source tools of HVML, please refer to the
following repositories:

- HVML Documents: <https://github.com/HVML/hvml-docs>.
- PurC (the Prime hVml inteRpreter for C language): <https://github.com/HVML/purc>.
- PurC Fetcher (the remote data fetcher for PurC): <https://github.com/HVML/purc-fetcher>.
- PurCMC (an HVML renderer in text-mode): <https://github.com/HVML/purc-midnight-commander>.
- xGUI Pro (an advanced HVML renderer based on WebKit): <https://github.com/HVML/xgui-pro>.

## Source Tree of PurC

PurC implements the parser, the interpreter, and some built-in dynamic variant
objects for HVML. It is mainly written in C/C++ language and will provide bindings
for Python and other script languages.

The source tree of PurC contains the following modules:

- `Source/PurC/include/`: The global header files.
- `Source/PurC/include/private`: The internal common header files.
- `Source/PurC/utils/`: Some basic and common utilities.
- `Source/PurC/instance/`: The operations of PurC instances and sessions.
- `Source/PurC/variant/`: The operations of variant.
- `Source/PurC/vcm/`: The operations of variant creation model tree.
- `Source/PurC/dvobjs/`: The dynamic variant objects.
- `Source/PurC/ejson/`: The eJSON parser. The eJSON parser reads an eJSON and constructs a variant creation model tree.
- `Source/PurC/dom/`: The operations of the DOM tree.
- `Source/PurC/vdom/`: The operations of the virtual DOM tree.
- `Source/PurC/html/`: The HTML parser. The HTML parser reads an HTML document or document fragements and constructs an eDOM tree.
- `Source/PurC/hvml/`: The HVML parser. The HTML parser reads an HVML document and constructs a vDOM tree.
- `Source/PurC/xgml/`: The XGML parser. The XGML parser reads an XGML document or document fragements and constructs an eDOM tree.
- `Source/PurC/xml/`: The XML parser. The XML parser parses an XML document or document fragements and constructs an eDOM tree.
- `Source/PurC/fetchers/`: The data fetchers to fetch data from various data sources (HTTP, FTP, and so on).
- `Source/PurC/listeners/`: The data listeners to listen events and/or send requests on various long-time connnection (hiDataBus, MQTT, WebSocket, and so on).
- `Source/PurC/executors/`: The internal/external executors.
- `Source/PurC/interpreter/`: The vDOM interpreter.
- `Source/PurC/ports/`: The ports for different operating systems, such as a POSIX-compliant system or Windows.
- `Source/PurC/bindings/`: The bindings for Python, Lua, and other programming languages.
- `Source/WTF/`: The simplified WTF (Web Template Framework) from WebKit.
- `Source/cmake/`: The cmake modules.
- `Source/ThirdParty/`: The third-party libraries, such as `gtest`.
- `Source/test/`: The unit test programs.
- `Source/Samples/`: Examples for using the interfaces of PurC.
- `Source/Tools/`: The tools (executables), i.e., the command line programs.
- `Source/Tools/purc`: The standalone HVML interpreter/debugger based-on PurC, which is an interactive command line program.
- `Documents/`: Some documents for developers.

Note that the HTML parser and DOM operations of PurC are derived from:

 - [Lexbor](https://github.com/lexbor/lexbor), which is licensed under
   the Apache License, Version 2.0.

## Current Status

This project was launched in June. 2021.

We welcome anybody to take part in the development and contribute your effort!

For the community conduct, please refer to [Code of Conduct](CODE_OF_CONDUCT.md).

For the coding style, please refer to [HybridOS-Code-and-Development-Convention](https://gitlab.fmsoft.cn/hybridos/hybridos/blob/master/docs/specs/HybridOS-Code-and-Development-Convention.md).

## Building

### Commands

To build:

```
rm -rf build && cmake -DCMAKE_BUILD_TYPE=Debug -DPORT=Linux -B build && cmake --build build
```

### Using the test samples


### Other documents


## Authors and Contributors

- R&D Team of [FMSoft Technologies]

## Copying

Copyright (C) 2021, 2022 [FMSoft Technologies]

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

Note that the software in `Source/Tools/` may use other open source licenses.
Please refer the COPYING file or LICENSE file for the licenses in
the source directories under `Source/Tools/`.

## Tradmarks

1) `HVML` is a registered tradmark of [FMSoft Technologies] in China and other contries or regions.

![HVML](https://www.fmsoft.cn/application/files/8116/1931/8777/HVML256132.jpg)

2) `呼噜猫` is a registered tradmark of [FMSoft Technologies] in China and other contries or regions.

![呼噜猫](https://www.fmsoft.cn/application/files/8416/1931/8781/256132.jpg)

3) `Purring Cat` is a tradmark of [FMSoft Technologies] in China and other contries or regions.

![Purring Cat](https://www.fmsoft.cn/application/files/2816/1931/9258/PurringCat256132.jpg)

4) `PurC` is a tradmark of [FMSoft Technologies] in China and other contries or regions.

![PurC](https://www.fmsoft.cn/application/files/5716/2813/0470/PurC256132.jpg)

[Beijing FMSoft Technologies Co., Ltd.]: https://www.fmsoft.cn
[FMSoft Technologies]: https://www.fmsoft.cn
[FMSoft]: https://www.fmsoft.cn
[HybridOS Official Site]: https://hybridos.fmsoft.cn
[HybridOS]: https://hybridos.fmsoft.cn

[HVML]: https://github.com/HVML
[MiniGUI]: http:/www.minigui.com
[WebKit]: https://webkit.org
[HTML 5.3]: https://www.w3.org/TR/html53/
[DOM Specification]: https://dom.spec.whatwg.org/
[WebIDL Specification]: https://heycam.github.io/webidl/
[CSS 2.2]: https://www.w3.org/TR/CSS22/
[CSS Box Model Module Level 3]: https://www.w3.org/TR/css-box-3/

[Vincent Wei]: https://github.com/VincentWei

[React.js]: https://reactjs.org
[Vue.js]: https://vuejs.org

[HVML Specifiction V1.0]: https://github.com/HVML/hvml-docs/blob/master/zh/hvml-spec-v1.0-zh.md
[HVML Predefined Variables V1.0]: https://github.com/HVML/hvml-docs/blob/master/zh/hvml-spec-predefined-variables-v1.0-zh.md

