# PurC

PurC is an hVml inteRpreter for C language. PurC is also the abbreviation of Purring Cat,
while Purring Cat is the nickname for the reference implementation of an HVML interpreter.

- [Introduction to HVML](#introduction-to-hvml)
- [Source Tree of PurC](#source-tree-of-purc)
- [Current Status](#current-status)
- [Building](#building)
   + [Commands](#commands)
   + [Using the test samples](#using-the-test-samples)
   + [Other documents](#other-documents)
- [Authors and Contributors](#authors-and-contributors)
- [Copying](#copying)
- [Tradmarks](#tradmarks)

## Introduction to HVML

With the development of Internet technology and applications, the Web front-end
development technology around HTML/CSS/JavaScript has evolved
rapidly, and it can even be described as "thousand miles in a day". Five years ago,
front-end frameworks based on jQuery and Bootstrap became popular. Since 2019,
frameworks based on virtual DOM (Document Object Model) technology have been favored 
by front-end developers, such as the famous React.js (https://reactjs.org/), 
Vue.js (https://cn.vuejs.org) etc. It is worth noting that WeChat
mini-programs and quick-apps etc, also use the virtual DOM technology
to build application frameworks at the same time.

The so-called "virtual DOM" refers to a front-end application that uses
JavaScript to create and maintain a virtual DOM tree.
Application scripts do not directly manipulate the real DOM tree.
In the virtual DOM tree, some process control based on data is realized
through some special attributes, such as conditions and loops.
virtual DOM technology provides the following benefits:

1. Because the script does not directly manipulate the real DOM tree. On the one hand, 
   the existing framework    simplifies the complexity of front-end development, 
   on the other hand,  it reduces the frequent operations on the DOM tree through 
   dynamic modification of page content by optimizing the operation of the real DOM tree, 
   thus improving page rendering efficiency and user experience.
   
2. With the virtual DOM technology, the modification of a certain data
   by the program can directly be reflected on the content of the data-bound page,
   and the developer does not need to actively or directly call the relevant
   interface to operate the DOM tree. This technology provides so-called
   "responsive" programming, which greatly reduces the workload of developers.

Front-end frameworks represented by React.js and Vue.js have achieved
great success, but have the following deficiencies and shortcomings:

1. These technologies are based on mature Web standards and require browsers
   that fully support the relevant front-end specifications to run, so they
   cannot be applied to other occasions. For example, if you want to use
   this kind of technology in Python scripts, there is currently no solution;
   another example is in traditional GUI application programming, you cannot benefit
   from this technology.
   
2. These technologies implement data-based conditions and loop flow controls
   by introducing virtual attributes such as `v-if`, `v-else`, and `v-for`. However,
   this method brings a sharp drop in code readability, which in turn brings drop of 
   code maintainability. Below is an example in Vue.js:

```html
<div v-if="Math.random() > 0.5">
  Now you see "{{ name }}"
</div>
<div v-else>
  Now you don't
</div>
```

During the development of [HybridOS](https://hybridos.fmsoft.cn),
[Vincent Wei](https://github.com/VincentWei) proposed a complete,
general purpose, elegant and easy-to-learn markup language, HVML (the
Hybrid Virtual Markup Language), based on the idea of virtual DOM.
HVML is a general purpose dynamic markup language, mainly used to generate
actual XML/HTML document content. HVML realizes the ability to
dynamically generate and update XML/HTML documents through
data-driven action tags and preposition attributes; HVML also provides
methods to integrate with existing programming languages, such as C/C++,
Python, Lua, and JavaScript, thus supporting more complex functions.

The classical `helloworld` program in HVML looks like:

```html
<!DOCTYPE hvml>
<hvml target="html">
    <head>
        <set on="$_T.map" to="replace" with="https://foo.bar/messages/$_SYSTEM.locale" />

        <title>Hello, world!</title>
    </head>

    <body>
        <p>$_T.get("Hello, world!")</p>
    </body>

</hvml>
```

Or,

```html
<!DOCTYPE hvml>
<hvml target="html">
    <head>
        <title>Hello, world!</title>

        <init as="messages">
            {
              "zh_CN" : "世界，您好！",
              "en_US" : "Hello, world!"
            }
        </init>
    </head>

    <body>
        <p>
            <choose on="$messages" to="update" by="KEY: $_SYSTEM.locale">
                <update on="$@" textContent="$?" />
                <except on="KeyError">
                    No valid locale defined.
                </except>
            </choose>
        </p>
    </body>
</hvml>
```

For more information about HVML, please refer to the documents in the following repository:

- [HVML Documents](https://gitlab.fmsoft.cn/hvml/hvml-docs)

## Source Tree of PurC

PurC implements the parser, the interpreter, and some built-in dynamic variant objects for HVML.
It is mainly written in C/C++ language and provides bindings for Python.

The source tree of PurC contains the following modules:

- `Source/PurC/include/`: The global header files.
- `Source/PurC/include/private`: The internal common header files.
- `Source/PurC/utils/`: Some basic and common utilities.
- `Source/PurC/instance/`: The operations of PurC instances and sessions.
- `Source/PurC/variant/`: The operations of variant.
- `Source/PurC/vcm/`: The operations of variant creation model tree.
- `Source/PurC/dvobjs/`: The dynamic variant objects.
- `Source/PurC/ejson/`: The eJSON parser. The eJSON parser reads a eJSON and constructs a variant creation model tree.
- `Source/PurC/edom/`: The operations of the effective DOM tree.
- `Source/PurC/vdom/`: The operations of the virtual DOM tree.
- `Source/PurC/html/`: The HTML parser. The HTML parser reads a HTML document and constructs a eDOM tree.
- `Source/PurC/hvml/`: The HVML parser. The HTML parser reads a HVML document and constructs a vDOM tree.
- `Source/PurC/xgml/`: The XGML parser. The XGML parser reads a XGML document and constructs a eDOM tree.
- `Source/PurC/xml/`: The XML parser. The XML parser reads a XML document and constructs a eDOM tree.
- `Source/PurC/fetchers/`: The data fetchers to fetch data from various data sources (HTTP, FTP, and so on).
- `Source/PurC/listeners/`: The data listeners to listen events and/or send requests on various long-time connnection (hiDataBus, MQTT, WebSocket, and so on).
- `Source/PurC/executors/`: The internal/external executors.
- `Source/PurC/interpreter/`: The vDOM interpreter.
- `Source/PurC/ports/`: The ports for different operating systems, such as a POSIX-compliant system or Windows.
- `Source/PurC/bindings/`: The bindings for Python, Lua, and other programming languages.
- `Source/ThirdParty/`: The third-party libraries.
- `Source/WTF/`: The simplified WTF (Web Template Framework) from WebKit.
- `Source/bmalloc/`: The `bmalloc` from WebKit.
- `Source/cmake/`: The cmake modules.
- `Source/test/`: The unit test programs.
- `Tools/`: The tools (executables), e.g., the command line program.
- `Documents/`: Some notes for developers.
- `Examples/`: Examples.

Note that the HTML parser and DOM operations of PurC are derived from:

 - [Lexbor](https://github.com/lexbor/lexbor), which is licensed under Apache 2.0.
 - [MyHTML](https://github.com/lexborisov/myhtml), which is licensed under LGPL 2.1.

## Current Status

This project was launched in June. 2021.

We welcome anybody to take part in the development and contribute your effort!

For the community conduct, please refer to [Code of Conduct](CODE_OF_CONDUCT.md).

For the coding style, please refer to [HybridOS-Code-and-Development-Convention](https://gitlab.fmsoft.cn/hybridos/hybridos/blob/master/docs/specs/HybridOS-Code-and-Development-Convention.md).

## Building

### Commands

To build:

```
rm -rf build && cmake -DCMAKE_BUILD_TYPE=Debug -DPORT=HybridOS -DUSE_LD_GOLD=OFF -B build && cmake --build build
```

### Using the test samples


### Other documents


## Authors and Contributors

- R&D Team of FMSoft (<https://www.fmsoft.cn>)

## Copying

Copyright (C) 2021 FMSoft (<https://www.fmsoft.cn>)

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

## Tradmarks

1) `HVML` is a registered tradmark of Beijing FMSoft Technologies. Co., Ltd. in China and other contries or regions.

![HVML](https://www.fmsoft.cn/application/files/8116/1931/8777/HVML256132.jpg)

2) `呼噜猫` is a registered tradmark of Beijing FMSoft Technologies. Co., Ltd. in China and other contries or regions.

![呼噜猫](https://www.fmsoft.cn/application/files/8416/1931/8781/256132.jpg)

3) `Purring Cat` is a registered tradmark of Beijing FMSoft Technologies. Co., Ltd. in China and other contries or regions.

![Purring Cat](https://www.fmsoft.cn/application/files/2816/1931/9258/PurringCat256132.jpg)

[Beijing FMSoft Technologies Co., Ltd.]: https://www.fmsoft.cn
[FMSoft Technologies]: https://www.fmsoft.cn
[FMSoft]: https://www.fmsoft.cn
[HybridOS Official Site]: https://hybridos.fmsoft.cn
[HybridOS]: https://hybridos.fmsoft.cn

[MiniGUI]: http:/www.minigui.com
[WebKit]: https://webkit.org
[HTML 5.3]: https://www.w3.org/TR/html53/
[DOM Specification]: https://dom.spec.whatwg.org/
[WebIDL Specification]: https://heycam.github.io/webidl/
[CSS 2.2]: https://www.w3.org/TR/CSS22/
[CSS Box Model Module Level 3]: https://www.w3.org/TR/css-box-3/

[Vincent Wei]: https://github.com/VincentWei
