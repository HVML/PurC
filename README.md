# PurC

`PurC` is the prime HVML interpreter for C language.

**Table of Contents**

[//]:# (START OF TOC)

- [Introduction](#introduction)
- [Building PurC](#building-purc)
   + [Prerequistes](#prerequistes)
   + [Building steps](#building-steps)
- [Using `purc`](#using-purc)
   + [Run a single HVML program](#run-a-single-hvml-program)
   + [Run multiple HVML programs at the same time](#run-multiple-hvml-programs-at-the-same-time)
   + [Use HVML renderer](#use-hvml-renderer)
   + [Run an HVML app in mutiple runners](#run-an-hvml-app-in-mutiple-runners)
   + [Sample HVML programs](#sample-hvml-programs)
- [Hacking PurC](#hacking-purc)
   + [Current Status](#current-status)
   + [Source Tree of PurC](#source-tree-of-purc)
   + [Running test programs](#running-test-programs)
   + [TODO List](#todo-list)
   + [Other documents](#other-documents)
- [Authors and Contributors](#authors-and-contributors)
- [Copying](#copying)
- [Tradmarks](#tradmarks)

[//]:# (END OF TOC)

## Introduction

`PurC` is the acronym of `the Prime HVML inteRpreter for C language`. It is also
the abbreviation of `Purring Cat`, while `Purring Cat` is the nickname
and the mascot of HVML, which is a new-style programming language proposed
by [Vincent Wei].

For more information about HVML, please refer to the article
_HVML, a Programable Markup Language_:

- [Link on GitHub](https://github.com/HVML/hvml-docs/blob/master/en/an-introduction-to-hvml-en.md)
- [Link on GitLab](https://gitlab.fmsoft.cn/hvml/hvml-docs/-/blob/master/en/an-introduction-to-hvml-en.md)

The goal of PurC is to implement all features defined by [HVML Specifiction V1.0]
and all predefined dynamic variables defined by [HVML Predefined Variables V1.0]
in C language.

You can use PurC to run a HVML program by using the command line tool `purc`, or
use PurC as a library to build your own HVML interpreter. We release PurC under
LGPLv3, so it is free for commercial use if you follow the conditions and terms
of LGPLv3.

PurC provides support for Linux and macOS. The support for Windows is
on the way. We welcome others to port PurC to other platforms.

For documents or other open source tools of HVML, please refer to the
following repositories:

- HVML Documents: <https://github.com/HVML/hvml-docs>.
- PurC (the Prime hVml inteRpreter for C language): <https://github.com/HVML/purc>.
- PurC Fetcher (the remote data fetcher for PurC): <https://github.com/HVML/purc-fetcher>.
- xGUI Pro (an advanced HVML renderer based on WebKit): <https://github.com/HVML/xgui-pro>.
- PurCMC (an HVML renderer in text-mode): <https://github.com/HVML/purc-midnight-commander>.

## Building PurC

### Prerequistes

To build PurC, make sure that the following tools or libraries are available
on your Linux or macOS system:

1. cmake
1. GCC 8.0 or later.
1. glib 2.44.0

### Building steps

After fetch the source of PurC, you can change to the root of the source treen.
Assume that you are using Linux, you can use the fillowing one command line to
build PurC:

```
    rm -rf build && cmake -DCMAKE_BUILD_TYPE=Debug -DPORT=Linux -B build && cmake --build build && sudo cmake --install build
```

The above command line consists of the following commands:

1. `rm -rf build`: Remove the old `build/` subdirectory if there is one.
Please make sure that you are really in the root the source tree of PurC.
1. `cmake -DCMAKE_BUILD_TYPE=Debug -DPORT=Linux -B build`: Change to the `build/`
subdirectory and run `cmake` to generate the building files to build PurC.
Note that this command uses the following options:
   - `-DCMAKE_BUILD_TYPE=Debug`: Specify the building type is `Debug`. You can
   also use `Release`, `RelWithDebInfo` and other options supported by `cmake`.
   - `-DPORT=Linux`: Tell `cmake` we are building PurC for an operating system
   based on Linux kernel.
   - `-B build`: Generate building files in `build/` subdirectory.
1. `cmake --build build`: Build PurC in `build/` subdirectory.
1. `sudo cmake --install build`: Install PurC from `build/` subdirectory.

You can also use the following commands to build and install PurC step by step:

```
    $ cd <path/to/the/root/of/the/source/tree/of/PurC>
    $ rm -rf build/
    $ mkdir build/
    $ cd build/
    $ cmake -DCMAKE_BUILD_TYPE=Release -DPORT=Linux
    $ make -j4
    $ sudo make install
```

By default, the above commands will build PurC and install the headers,
libraries, executables, and some documents to your system (under `/usr/local/`
directory if you are using Linux system).

## Using `purc`

The following sections assume that you have installed `purc` to your system.

### Run a single HVML program

Please save the following contents in a file named `hello.hvml` as your
first HVML program in your working directory:

```html
<!DOCTYPE hvml>
<hvml target="void">

    $STREAM.stdout.writelines('Hello, world!')

</hvml>
```

To run this HVML program, you can use `purc` in the following way:

```bash
    $ purc hello.hvml
```

You will see that your first HVML program prints `Hello, world!`
on your terminal and quit:

```
    Hello, world!
```

You can also run this HVML program directly as a script if you prepend the
following line as the first line in your HVML program:

```
    #!/usr/local/bin/purc
```

After this, run the following command to change the mode of the file to have
the execute permission:

```bash
    $ chmod +x hello.hvml
```

then run `hello.hvml` directly from the command line:

```bash
    $ ./hello.hvml
```

### Run multiple HVML programs at the same time

PurC can run multiple HVML programs as coroutines at the same time.

For example, we enhance the first HVML program to print `Hello, world!` 10 times:

```html
<!DOCTYPE hvml SYSTEM 'v: MATH'>
<hvml target="void">
    <iterate on 0 onlyif $L.lt($0<, 10) with $MATH.add($0<, 1) >
        $STREAM.stdout.writelines(
                $STR.join($0<, ") Hello, world! --from COROUTINE-", $HVML.cid))
    </iterate>
</hvml>
```

Assume you named the enhanced version as `hello-10.hvml`, we can run
the program in two coroutines at the same time:

```bash
    $ purc hello-10.hvml hello-10.hvml
```

You will see the following output on your terminal:

```
0) Hello, world! -- from COROUTINE-01
0) Hello, world! -- from COROUTINE-02
1) Hello, world! -- from COROUTINE-01
1) Hello, world! -- from COROUTINE-02
2) Hello, world! -- from COROUTINE-01
2) Hello, world! -- from COROUTINE-02
3) Hello, world! -- from COROUTINE-01
3) Hello, world! -- from COROUTINE-02
4) Hello, world! -- from COROUTINE-01
4) Hello, world! -- from COROUTINE-02
5) Hello, world! -- from COROUTINE-01
5) Hello, world! -- from COROUTINE-02
6) Hello, world! -- from COROUTINE-01
6) Hello, world! -- from COROUTINE-02
7) Hello, world! -- from COROUTINE-01
7) Hello, world! -- from COROUTINE-02
8) Hello, world! -- from COROUTINE-01
8) Hello, world! -- from COROUTINE-02
9) Hello, world! -- from COROUTINE-01
9) Hello, world! -- from COROUTINE-02
```

In the above output, `COROUTINE-01` and `COROUTINE-02` contain the coroutine
identifier allocated by PurC for two running instances of the program.
You see that PurC schedules the running instances to execute alternately, i.e.,
in the manner of coroutines.

### Use HVML renderer

Now, we want our HVML program generate a HTML file instead of printing to
the terminal. So we can open the genenrated HTML file in a web browser.

Therefore, we enhance `hello-10.hvml` once more:

```html
<!DOCTYPE hvml SYSTEM 'v: MATH'>
<hvml target="html">
    <head>
        <title>Hello, world!</title>
    <head>

    <body>
        <ul>
            <iterate on 0 onlyif=$L.lt($0<, 10) with=$MATH.add($0<, 1) >
                <li>$< Hello, world! --from COROUTINE-$HVML.cid</li>
            </iterate>
        </ul>
    </body>
</hvml>
```

and save the contents in `hello-html.hvml` file. Note that there are two key
differences:

1. The value of `target` attribute of `hvml` element changes to `html`.
1. We used HTML tags such as `head`, `body`, `ul`, and `li`.

If we run `hello-html.hvml` program by using `purc`, we need to specify some
information about renderer. By default, `purc` uses the renderer called
`HEADLESS`. This renderer just prints the generated HTML contents to a file:

```bash
    $ purc --rdr-uri=file:///tmp/hello.html hello-html.hvml
```

In the above command line, we use `--rdr-uri` to specify the file.

After running the command, the contents in `/tmp/hello.html` looks like:

```html
<!DOCTYPE html>
<html>
    <head>
        <title>Hello, world!</title>
    <head>

    <body>
        <ul>
            <li>0) Hello, world! -- from COROUTINE-01</li>
            <li>1) Hello, world! -- from COROUTINE-01</li>
            <li>2) Hello, world! -- from COROUTINE-01</li>
            <li>3) Hello, world! -- from COROUTINE-01</li>
            <li>4) Hello, world! -- from COROUTINE-01</li>
            <li>5) Hello, world! -- from COROUTINE-01</li>
            <li>6) Hello, world! -- from COROUTINE-01</li>
            <li>7) Hello, world! -- from COROUTINE-01</li>
            <li>8) Hello, world! -- from COROUTINE-01</li>
            <li>9) Hello, world! -- from COROUTINE-01</li>
        </ul>
    </body>
</html>
```

You can also direct `purc` to connect to a real renderer, for example, `xGUI Pro`.
It is an advanced HVML renderer based on WebKit.

Assume that you have installed xGUI Pro on your system (please refer to
<https://github.com/HVML/xgui-pro> for detailed instructions to install xGUI Pro),
you can run `purc` with the following options to show the ultimate HTML contents
in a window of xGUI Pro:

```bash
    $ purc --rdr-protocol=purcmc hello-html.hvml
```

You can see the all options supported by `purc` when you run `purc` with `-h` option:

```bash
$ purc -h
purc (0.2.0) - a standalone HVML interpreter/debugger based-on PurC.

Usage: purc [ options ... ] [ file | url ] ... | [ app_desc_json | app_desc_ejson ]

The following options can be supplied to the command:

  -a --app=< app_name >
        Execute with the specified app name (default value is `cn.fmsoft.html.purc`).

  -r --runner=< runner_name >
        Execute with the specified runner name (default value is `main`).

  -d --data-fetcher=< local | remote >
        The data fetcher; use `local` or `remote`.
            - `local`: use the built-in data fetcher, and only `file://` URIs
               supported.
            - `remote`: use the remote data fetcher to support more URL schemas,
               such as `http`, `https`, `ftp` and so on.

  -p --rdr-prot=< headless | purcmc >
        The renderer protocol; use `headless` (default) or `purcmc`.
            - `headless`: use the built-in HEADLESS renderer.
            - `purcmc`: use the remote PURCMC renderer;
              `purc` will connect to the renderer via Unix Socket or WebSocket.

  -u --rdr-uri=< renderer_uri >
        The renderer URI:
            - For the renderer protocol `headleass`,
              default value is not specified (nil).
            - For the renderer protocol `purcmc`,
              default value is `unix:///var/tmp/purcmc.sock`.

  -t --request=< json_file | - >
        The JSON file contains the request data which will be passed to
        the HVML programs; use `-` if the JSON data will be given through
        stdin stream.

  -q --quiet
        Execute the program quietly (without redundant output).

  -c --copying
        Display detailed copying information and exit.

  -v --version
        Display version information and exit.

  -h --help
        This help.
```

### Run an HVML app in mutiple runners

PurC supports to run an app in multiple runners. For this purpose, you should
prepare a JSON file or an eJSON file which defines the app, the runners, and
the initial HVML programs to run in different runners.

```json
{
    "name": "cn.fmsoft.hvml.sample",
    "runners": [
        {
            "name": "Products",
            "renderer": { "protocol": "purcmc", "uri": "unix:///var/tmp/purcmc.sock" },
            "workspace": { "name": "default", "layout": "cn.fmsoft.hvml.sample/layout.html" },
            "coroutines": [
                { "uri": "cn.fmsoft.hvml.sample/productlist.hvml", "request": {},
                   "renderer": { "pageType": "widget", "pageName": "productlist", "pageGroupId": "theProductsArea" }
                },
                { "uri": "cn.fmsoft.hvml.sample/productinfo.hvml", "request": { "productId": 0 },
                   "renderer": { "pageType": "widget", "pageName": "productinfo", "pageGroupId": "theProductsArea" }
                }
            ]
        },
        {
            "name": "Customers",
            "renderer": { "protocol": "purcmc", "uri": "unix:///var/tmp/purcmc.sock" },
            "workspace": { "name": "default", "layout": "cn.fmsoft.hvml.sample/layout.html" },
            "coroutines": [
                { "uri": "cn.fmsoft.hvml.sample/customerlist.hvml", "request": {},
                   "renderer": { "pageType": "widget", "pageName": "customerlist", "pageGroupId": "theCustomersArea" }
                },
                { "uri": "cn.fmsoft.hvml.sample/customerlist.hvml", "request": { "customerId": 0 },
                   "renderer": { "pageType": "widget", "pageName": "customerinfo", "pageGroupId": "theCustomersArea" }
                }
            ]
        },
        {
            "name": "Daemons",
            "coroutines": [
                { "uri": "cn.fmsoft.hvml.sample/check-customers.hvml", "request": { "interval": 10 } },
                { "uri": "cn.fmsoft.hvml.sample/check-products.hvml", "request": { "interval": 30 } }
            ]
        },
    ]
```

Assume that you prepare all HVML programs and save the above JSON as
`cn.fmsoft.hvml.sample.json`, you can run `purc` in the following way:

```
    $ purc cn.fmsoft.hvml.sample.json
```

to start the HVML app.

Note that, when running an app in this way, you can access the command line
options in the eJSON file through the variable `$OPTS` prepared by `purc` when
parsing the eJSON file.

This gives a typical application of parameterized eJSON introduced by PurC.

For example, we can specified the command line options:

```bash
$ purc --app=cn.fmsoft.hvml.sample my_app.ejson
```

We can access the option specified by `--app` in `my_app.ejson`:

```json
{
    "name": "$OPTS.app",
    "runners": [
        {
            "name": "Products",
            "renderer": { "protocol": "purcmc", "uri": "unix:///var/tmp/purcmc.sock" },
            "workspace": { "name": "default", "layout": "$OPTS.app/layout.html" },
            "coroutines": [
                { "uri": "cn.fmsoft.hvml.sample/productlist.hvml", "request": {},
                   "renderer": { "pageType": "widget", "pageName": "productlist", "pageGroupId": "theProductsArea" }
                },
                { "uri": "cn.fmsoft.hvml.sample/productinfo.hvml", "request": { "productId": 0 },
                   "renderer": { "pageType": "widget", "pageName": "productinfo", "pageGroupId": "theProductsArea" }
                },
            ]
        },
    ]
}
```

All occurrences of `$OPTS.app` in `my_app.ejson` will be subsituted by
`cn.fmsoft.hvml.sample`.

### Sample HVML programs

You can find more sample HVML programs in respository
[HVML Documents](https://github.com/HVML/hvml-docs), under the directory `samples/`.

You can use `purc` to run the sample directly:

```bash
$ purc https://github.com/HVML/hvml-docs/raw/master/samples/fibonacci/fibonacci-6.hvml
```

If the firewall refused to connect to the URL, use the following URL:

```bash
$ purc https://gitlab.fmsoft.cn/hvml/hvml-docs/-/raw/master/samples/fibonacci/fibonacci-6.hvml
```

When `purc` try to load a HVML program from the remote URL, it will enable
the remote data fetcher by default. Please refer to
[PurC Fetcher](https://github.com/HVML/purc-fetcher) for detailed instructions
to build and install PurC Fetcher to your system.

## Hacking PurC

### Current Status

This project was launched in June. 2021. This is the version 0.8.0 of PurC.

After one year development, the current version implements all features
defined by [HVML Specifiction V1.0] in C language, and also implements all
predefined dynamic variables defined by [HVML Predefined Variables V1.0].

We welcome anybody to take part in the development and contribute your effort!

### Source Tree of PurC

PurC implements the parser, the interpreter, and some built-in dynamic variant
objects for HVML. It is mainly written in C/C++ language and will provide bindings
for Python and other script languages in the future.

The source tree of PurC contains the following modules:

- `Source/PurC/include/`: The global header files.
- `Source/PurC/include/private`: The internal common header files.
- `Source/PurC/utils/`: Some basic and common utilities.
- `Source/PurC/variant/`: The implementation of variant.
- `Source/PurC/vcm/`: The operations of variant creation model tree.
- `Source/PurC/dvobjs/`: The built-in dynamic variant objects.
- `Source/PurC/ejson/`: The implementation of the eJSON parser. The eJSON parser reads an eJSON and constructs a variant creation model tree.
- `Source/PurC/dom/`: The implentation of the DOM tree.
- `Source/PurC/vdom/`: The implementation of the virtual DOM tree.
- `Source/PurC/html/`: The implementation of the HTML parser. The HTML parser reads an HTML document or document fragements and constructs an eDOM tree.
- `Source/PurC/hvml/`: The implementation of the HVML parser. The HTML parser reads an HVML document and constructs a vDOM tree.
- `Source/PurC/xgml/`: The implementation of the XGML parser (Not implemented so far). The XGML parser reads an XGML document or document fragements and constructs an eDOM tree.
- `Source/PurC/xml/`: The XML parser (Not implemented so far). The XML parser parses an XML document or document fragements and constructs an eDOM tree.
- `Source/PurC/instance/`: The operations of PurC instances and sessions.
- `Source/PurC/fetchers/`: The data fetchers to fetch data from various data sources (FILE, HTTP, FTP, and so on).
- `Source/PurC/executors/`: The implementation of internal executors.
- `Source/PurC/interpreter/`: The vDOM interpreter.
- `Source/PurC/pcrdr/`: The management of connection to the renderer.
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

### Running test programs


### TODO List

1. More tests or test cases.
1. More samples.
1. Port PurC to Windows.

For the community conduct, please refer to [Code of Conduct](CODE_OF_CONDUCT.md).

For the coding style, please refer to [HybridOS-Code-and-Development-Convention](https://gitlab.fmsoft.cn/hybridos/hybridos/blob/master/docs/specs/HybridOS-Code-and-Development-Convention.md).


### Other documents


## Authors and Contributors

- Vincent Wei
- Nine Xue
- XU Xiaohong
- LIU Xin
- GENG Yue

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

