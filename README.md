# PurC

`PurC` is the prime HVML interpreter for C language.

**Table of Contents**

[//]:# (START OF TOC)

- [Introduction](#introduction)
- [Building PurC](#building-purc)
- [Using `purc`](#using-purc)
- [Hacking PurC](#hacking-purc)
- [Authors and Contributors](#authors-and-contributors)
- [Copying](#copying)
- [Tradmarks](#tradmarks)

[//]:# (END OF TOC)

## Introduction

`HVML` is a new-style and general-purpose programming language proposed by [Vincent Wei],
    who is the author of the China-first open source project - [MiniGUI].

`PurC` is the acronym of `the Prime HVML inteRpreter for C language`.
It is also the abbreviation of `Purring Cat`,
   while `Purring Cat` is the nickname and the mascot of HVML.

The goal of PurC is to implement all features defined by [HVML Specifiction V1.0]
and all predefined dynamic variables defined by [HVML Predefined Variables V1.0] in C language.

You can use PurC to run an HVML program or an HVML app by using the command line tool `purc`,
    or use PurC as a library to build your own HVML interpreter.

We release PurC under LGPLv3, so it is free for commercial use if you follow the conditions and terms of LGPLv3.

By now, PurC provides support for Linux and macOS.
The support for Windows is on the way.
We welcome anyone to port PurC to other platforms.

To learn the basic concepts about HVML programming, please refer to the following tutorial:

- [Learn HVML Programming in 30 Minutes](https://github.com/HVML/hvml-docs/blob/master/en/learn-hvml-programming-in-30-minutes-en.md)

For more information about HVML, please refer to the article (10% complete):

- [HVML, a Programable Markup Language](https://github.com/HVML/hvml-docs/blob/master/en/an-introduction-to-hvml-en.md)

For specifications and open source software related to HVML, please refer to the following repositories:

- HVML Documents: <https://github.com/HVML/hvml-docs>.
- PurC (the Prime hVml inteRpreter for C language): <https://github.com/HVML/PurC>.
- PurC Fetcher (the remote data fetcher for PurC): <https://github.com/HVML/PurC-Fetcher>.
- DOM Ruler (A library to maintain a DOM tree, lay out and stylize the DOM elements by using CSS): <https://github.com/HVML/DOM-Ruler>.
- xGUI Pro (an advanced HVML renderer based on WebKit): <https://github.com/HVML/xGUI-Pro>.
- PurC Midnight Commander (an HVML renderer in text-mode): <https://github.com/HVML/PurC-Midnight-Commander>.

## Building PurC

### Prerequistes

To build PurC, make sure that the following tools or libraries are available on your Linux or macOS system:

1. cmake
1. A C11 and CXX17 compliant complier: GCC 8+ or Clang 6+
1. glib 2.44.0 or later
1. Python 3
1. BISON 3.0 or later
1. FLEX 2.6.4 or later

Although the port for Windows is still on the way, it is possible to build PurC on Windows 10 version 2004 or later:
You can install WSL (Windows Subsystem for Linux) and a Linux distribution, e.g., Ubuntu, on your Windows system,
    then build PurC in Ubuntu environment.

### Building steps

We assume that you are using Linux.

After fetched the source of PurC, you can change to the root of the source treen,
      and use the following command line to build and install PurC:

```
$ cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPORT=Linux -B build && cmake --build build && sudo cmake --install build
```

The above command line consists of the following commands:

1. `cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPORT=Linux -B build`: Change to the `build/`
subdirectory and run `cmake` to generate the building files to build PurC.
Note that this command uses the following options:
   - `-DCMAKE_BUILD_TYPE=RelWithDebInfo`: Specify the building type is `RelWithDebInfo`.
   You can also use `Debug`, `Release` and other options supported by `cmake`.
   - `-DPORT=Linux`: Tell `cmake` you are building PurC for an operating system
   based on Linux kernel. Use `-DPORT=Mac` if you are using macOS.
   - `-B build`: Generate building files in `build/` subdirectory.
1. `cmake --build build`: Build PurC in `build/` subdirectory.
1. `sudo cmake --install build`: Install PurC from `build/` subdirectory.

You can also use the following commands to build and install PurC step by step:

```bash
$ cd <path/to/the/root/of/the/source/tree/of/PurC>
$ rm -rf build/
$ mkdir build/
$ cd build/
$ cmake -DCMAKE_BUILD_TYPE=Release -DPORT=Linux ..
$ make -j4
$ sudo make install
```

If you'd like to use `ninja` instead of `make` to build PurC,
   you can use the following command:

```bash
$ cd <path/to/the/root/of/the/source/tree/of/PurC>
$ cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPORT=Linux -Bbuild -GNinja &&  ninja -Cbuild && ninja -Cbuild install
```

Note that you might need to remove `build/` directory first if there is already one.

By default, the above commands will build PurC and install the headers, libraries, executables,
   and some documents to your system (under `/usr/local/` directory if you are using Linux system).

When using `make`, you can use `DESTDIR` to specify an alternative installation directory:

```bash
$ make DESTDIR=/package/stage install
```

When using `ninja`, you can also use `DESTDIR` to specify an alternative installation directory:

```bash
$ DESTDIR="/package/stage" ninja -Cbuild install
```

## Environment Variables

PurC uses the following environment variables for different purposes:

- `PURC_DVOBJS_PATH`: the path to save the shared modules for external dynamic objects.
- `PURC_EXECUTOR_PATH`: the path to save the shared modules for external executors.
- `PURC_FETCHER_EXEC_PATH`: the path to save the executable program of PurC Fetcher.
- `PURC_USER_DIR_SUFFIX`: The directory suffix for user.
- `PURC_LOG_ENABLE`: `true` if enable the global log facility.
- `PURC_LOG_SYSLOG`: `true` if enable to use syslog as the log facility.

## Using `purc`

The following sections assume that you have installed PurC to your system,
    and the command line tool `purc` has been installed into `/usr/local/bin/`.
Make sure that you have added `/usr/local/lib` to `/etc/ld.so.conf` and run `sudo ldconfig` command,
     in order that the system can find the shared library of PurC you just installed into `/usr/local/lib`.

### Run a single HVML program

Please save the following contents in a file named `hello.hvml` as your
first HVML program in your working directory:

```hvml
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
the executing permission:

```bash
$ chmod +x hello.hvml
```

then run `hello.hvml` directly from the command line:

```bash
$ ./hello.hvml
```

### Run multiple HVML programs in parallel

PurC can run multiple HVML programs as coroutines in parallel.

For example, we enhance the first HVML program to print `Hello, world!` 10 times:

```hvml
<!DOCTYPE hvml>
<hvml target="void">
    <iterate on 0 onlyif $L.lt($0<, 10) with $EJSON.arith('+', $0<, 1) nosetotail >
        $STREAM.stdout.writelines(
                $STR.join($0<, ") Hello, world! --from COROUTINE-", $CRTN.cid))
    </iterate>
</hvml>
```

Assume you named the enhanced version as `hello-10.hvml`,
       we can run the two program as two coroutines in parallel by specifying the command line flag `-l`:

```bash
$ purc -l hello-10.hvml hello-10.hvml
```

You will see the following output on your terminal:

```
0) Hello, world! -- from COROUTINE-3
0) Hello, world! -- from COROUTINE-4
1) Hello, world! -- from COROUTINE-3
1) Hello, world! -- from COROUTINE-4
2) Hello, world! -- from COROUTINE-3
2) Hello, world! -- from COROUTINE-4
3) Hello, world! -- from COROUTINE-3
3) Hello, world! -- from COROUTINE-4
4) Hello, world! -- from COROUTINE-3
4) Hello, world! -- from COROUTINE-4
5) Hello, world! -- from COROUTINE-3
5) Hello, world! -- from COROUTINE-4
6) Hello, world! -- from COROUTINE-3
6) Hello, world! -- from COROUTINE-4
7) Hello, world! -- from COROUTINE-3
7) Hello, world! -- from COROUTINE-4
8) Hello, world! -- from COROUTINE-3
8) Hello, world! -- from COROUTINE-4
9) Hello, world! -- from COROUTINE-3
9) Hello, world! -- from COROUTINE-4
```

In the above output, `COROUTINE-3` and `COROUTINE-4` contain the coroutine identifier assigned by PurC for two running instances of the program.
You see that PurC schedules the running instances to execute alternately, i.e., in the manner of coroutines.

If you do not use the flag `-l` in the command line, `purc` will run the programs one by one:

```
$ purc hello-10.hvml hello-10.hvml
0) Hello, world! -- from COROUTINE-3
1) Hello, world! -- from COROUTINE-3
2) Hello, world! -- from COROUTINE-3
3) Hello, world! -- from COROUTINE-3
4) Hello, world! -- from COROUTINE-3
5) Hello, world! -- from COROUTINE-3
6) Hello, world! -- from COROUTINE-3
7) Hello, world! -- from COROUTINE-3
8) Hello, world! -- from COROUTINE-3
9) Hello, world! -- from COROUTINE-3
0) Hello, world! -- from COROUTINE-4
1) Hello, world! -- from COROUTINE-4
2) Hello, world! -- from COROUTINE-4
3) Hello, world! -- from COROUTINE-4
4) Hello, world! -- from COROUTINE-4
5) Hello, world! -- from COROUTINE-4
6) Hello, world! -- from COROUTINE-4
7) Hello, world! -- from COROUTINE-4
8) Hello, world! -- from COROUTINE-4
9) Hello, world! -- from COROUTINE-4
```

### Connecting to HVML renderer

One of important differences between HVML and other programming languages is that HVML can generate documents described in markup languages like HTML,
    not just output data to a file or your terminal.

For your convenience, we have prepared some HVML samples in the directory `Source/Samples/hvml` of this repository.
After building PurC, the samples will be copied to the building root directory, under `hvml/` subdirectroy,
      so that you can change to the building root directory and use `purc` to run the samples.
For example:

```bash
$ cd <path/to/the/building/directory/>
$ purc hvml/fibonacci-void-temp.hvml
```

This HVML program will output 18 Fibonacci numbers less than 2000.
There is also another version of this program: `hvml/fibonacci-html-temp.hvml`.
It will generate an HTML document listing the Fibonacci numbers.

If you run `hvml/fibonacci-html-temp.hvml` program by using `purc` without any option,
   `purc` will use the renderer called `HEADLESS`.
This renderer will record the messages sent by PurC to the renderer to a local file,
     it is `/dev/null` by default on Linux.
Because this HVML program did not use `$STREM.stdout` any more, you will see nothing on your terminal.
But you can use the option `--verbose` (or the short option `-b`) to show the HTML contents generated by the HVML program in your terminal:

```bash
$ purc -b hvml/fibonacci-html-temp.hvml
```

The command will give you the following output:

```
purc 0.8.0
Copyright (C) 2022 FMSoft Technologies.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

Executing HVML program from `file:///srv/devel/hvml/purc/build/hvml/fibonacci-html-temp.hvml`...

>> The document generated:
<html>
  <head>
    <title>
      Fibonacci Numbers
    </title>
  </head>
  <body id="theBody">
    <h1>
      Fibonacci Numbers less than 2000
    </h1>
    <ol>
      <li> 0 </li>
      <li> 1 </li>
      <li> 1 </li>
      <li> 2 </li>
      <li> 3 </li>
      <li> 5 </li>
      <li> 8 </li>
      <li> 13 </li>
      <li> 21 </li>
      <li> 34 </li>
      <li> 55 </li>
      <li> 89 </li>
      <li> 144 </li>
      <li> 233 </li>
      <li> 377 </li>
      <li> 610 </li>
      <li> 987 </li>
      <li> 1597 </li>
    </ol>
    <p>
      Totally 18 numbers
    </p>
  </body>
</html>


>> The executing result:
[18, 1597L]
```

You can also direct `purc` to connect to a real renderer, for example, `xGUI Pro`.
It is an advanced HVML renderer based on WebKit.

Assume that you have installed xGUI Pro on your system,
       you can run `purc` to show the ultimate HTML contents in a window of xGUI Pro.
Please refer to <https://github.com/HVML/xGUI-Pro> for detailed instructions to install xGUI Pro.

Assume that you have started xGUI Pro from another terminal, then please run `purc` with the following options:

```bash
$ purc --rdr-prot=purcmc hvml/fibonacci-html-temp-rdr.hvml
```

Note that, in the above command line, we execute a modified version of Fibonacci Numbers: `hvml/fibonacci-html-temp-rdr.hvml`.
If you compare these two versions, you will find that there is an `observe` element in the modified version:

```hvml
        <observe on $CRTN for "rdrState:pageClosed">
            <exit with [$count, $last_two] />
        </observe>
```

If there is no such `observe` element, the HVML program will exit immediately after generated the HTML document.
By using the `observe` element, the HVML program will wait for the time when the page created by the renderer is closed by the user (that is you).

You will see that the contents in a window of xGUI Pro created by `hvml/fibonacci-html-temp-rdr.hvml`:

![fibonacci-html-temp](https://files.fmsoft.cn/hvml/screenshots/fibonacci-html-temp.png)

If you close the window by clicking the close box on the caption bar,
   the HTML program will exit as normally.

For a complete HVML program which gives a better experience,
    you can try to run another sample called `hvml/calculator-bc.hvml`, which implements an arbitrary precision calculator:

```bash
$ purc -p purcmc hvml/calculator-bc.hvml
```

Here is the screenshot of `hvml/calculator-bc.hvml`:

![the Arbitrary Precision Calculator](https://files.fmsoft.cn/hvml/screenshots/calculator-bc.png)

Or run `hvml/planetary-resonance-lines.hvml`, which shows the Planetary Resonance:

```bash
$ purc -p purcmc hvml/planetary-resonance-lines.hvml
```

Here is the screenshot of `hvml/planetary-resonance-lines.hvml`:

![the Planetary Resonance](https://files.fmsoft.cn/hvml/screenshots/planetary-resonance.png)


### Options for `purc`

You can see the all options supported by `purc` when you run `purc` with `-h` option:

```bash
$ purc -h
purc (0.8.0) - a standalone HVML interpreter/debugger based-on PurC.

Usage: purc [ options ... ] [ file | url ] ... | [ app_desc_json | app_desc_ejson ]

The following options can be supplied to the command:

  -a --app=< app_name >
        Execute with the specified app name (default value is `cn.fmsoft.html.purc`).

  -r --runner=< runner_name >
        Execute with the specified runner name (default value is `main`).

  -d --data-fetcher=< local | remote >
        The data fetcher; use `local` or `remote`.
            - `local`: use the built-in data fetcher, and only `file://` URLs
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
              default value is `file:///dev/null`.
            - For the renderer protocol `purcmc`,
              default value is `unix:///var/tmp/purcmc.sock`.

  -t --request=< json_file | - >
        The JSON file contains the request data which will be passed to
        the HVML programs; use `-` if the JSON data will be given through
        stdin stream.

  -l --parallel
        Execute multiple programs in parallel.

  -s --verbose
        Execute the program(s) with verbose output.

  -c --copying
        Display detailed copying information and exit.

  -v --version
        Display version information and exit.

  -h --help
        This help.
```

### Run an HVML app in mutiple runners

PurC supports to run an app in multiple runners.
Here one `runner` is one thread in the `purc` process.

For this purpose, you can prepare a JSON file or an eJSON file which defines the app, the runners,
    and the initial HVML programs to run as coroutines in different runners.

The following is a sample:

```json
{
    "app": "cn.fmsoft.hvml.sample",
    "runners": [
        {
            "runner": "Products",
            "renderer": { "protocol": "purcmc", "uri": "unix:///var/tmp/purcmc.sock",
                "workspaceName": "default", "workspaceLayout": "cn.fmsoft.hvml.sample/layout.html" },
            "coroutines": [
                { "url": "cn.fmsoft.hvml.sample/productlist.hvml", "request": {},
                   "renderer": { "pageType": "widget", "pageName": "productlist", "pageGroupId": "theProductsArea" }
                },
                { "url": "cn.fmsoft.hvml.sample/productinfo.hvml", "request": { "productId": 0 },
                   "renderer": { "pageType": "widget", "pageName": "productinfo", "pageGroupId": "theProductsArea" }
                }
            ]
        },
        {
            "runner": "Customers",
            "renderer": { "protocol": "purcmc", "uri": "unix:///var/tmp/purcmc.sock",
                "workspaceName": "default", "workspaceLayout": "cn.fmsoft.hvml.sample/layout.html" },
            "coroutines": [
                { "url": "cn.fmsoft.hvml.sample/customerlist.hvml", "request": {},
                   "renderer": { "pageType": "widget", "pageName": "customerlist", "pageGroupId": "theCustomersArea" }
                },
                { "url": "cn.fmsoft.hvml.sample/customerlist.hvml", "request": { "customerId": 0 },
                   "renderer": { "pageType": "widget", "pageName": "customerinfo", "pageGroupId": "theCustomersArea" }
                }
            ]
        },
        {
            "runner": "Daemons",
            "coroutines": [
                { "url": "cn.fmsoft.hvml.sample/check-customers.hvml", "request": { "interval": 10 } },
                { "url": "cn.fmsoft.hvml.sample/check-products.hvml", "request": { "interval": 30 } }
            ]
        },
    ]
}
```

Assume that you prepare all HVML programs and save the above JSON as `cn.fmsoft.hvml.sample.json`,
       you can run `purc` in the following way:

```bash
$ purc cn.fmsoft.hvml.sample.json
```

Note that, when running an app in this way,
     you can use the command line options in the eJSON file through the variable `$OPTS` prepared by `purc` when parsing the eJSON file.
This gives a typical application of parameterized eJSON introduced by PurC.

For example, we can specified the command line options:

```bash
$ purc --app=cn.fmsoft.hvml.sample my_app.ejson
```

Then use the option specified by `--app` in `my_app.ejson`:

```json
{
    "app": "$OPTS.app",
    "runners": [
        {
            "runner": "Products",
            "renderer": { "protocol": "purcmc", "uri": "unix:///var/tmp/purcmc.sock",
                "workspaceName": "default", "workspaceLayout": "$OPTS.app/layout.html" },
            "coroutines": [
                { "url": "cn.fmsoft.hvml.sample/productlist.hvml", "request": {},
                   "renderer": { "pageType": "widget", "pageName": "productlist", "pageGroupId": "theProductsArea" }
                },
                { "url": "cn.fmsoft.hvml.sample/productinfo.hvml", "request": { "productId": 0 },
                   "renderer": { "pageType": "widget", "pageName": "productinfo", "pageGroupId": "theProductsArea" }
                },
            ]
        },
    ]
}
```

All occurrences of `$OPTS.app` in `my_app.ejson` will be subsituted by `cn.fmsoft.hvml.sample`.

### More HVML samples

You can find more HVML sample programs in respository [HVML Documents](https://github.com/HVML/hvml-docs),
    under the directory `samples/`.

You can use `purc` to run a sample resided in the remote HVML Documents repository directly:

```bash
$ purc --data-fetcher=remote https://github.com/HVML/hvml-docs/raw/master/samples/fibonacci/fibonacci-6.hvml
```

If the firewall refused to connect to the URL, use the following URL:

```bash
$ purc --data-fetcher=remote https://gitlab.fmsoft.cn/hvml/hvml-docs/-/raw/master/samples/fibonacci/fibonacci-6.hvml
```

Note that when `purc` try to load an HVML program from a remote URL,
     it will use the remote data fetcher by default.
Therefore, you must install PurC Fetcher in advance.
Please refer to [PurC Fetcher](https://github.com/HVML/purc-fetcher) for detailed instructions to build and install PurC Fetcher to your system.

## Hacking PurC

### Current Status

This project was launched in June. 2021. This is the version 0.8.0 of PurC.

The main purpose of PurC is providing a library for you to write your own HVML interpreter.
After one year development, the current version implements almost all features defined by [HVML Specifiction V1.0],
      and also implements almost all predefined dynamic variables defined by [HVML Predefined Variables V1.0].

Except for the HVML interpreter, PurC also provides many fundamental features for general C programs:

1. PurC provides the APIs for variant management, here variant is way of HVML program to manage data.
1. PurC provides the APIs for parsing JSON and extended JSON.
1. PurC provides the APIs for parsing and evaluting a parameterized eJSON expression.
1. PurC provides the APIs for parsing an HTML document.

You can use these groups of APIs independently according to your needs.

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
- `Source/ExtDVObjs/math/`: The implementation of the external dynamic variant object `$MATH`.
- `Source/ExtDVObjs/fs/`: The implementation of the external dynamic variant object `$FS` and `$FILE`.
- `Source/WTF/`: The simplified WTF (Web Template Framework) from WebKit.
- `Source/cmake/`: The cmake modules.
- `Source/ThirdParty/`: The third-party libraries, such as `gtest`.
- `Source/test/`: The unit test programs.
- `Source/Samples/api`: Samples for using the API of PurC.
- `Source/Samples/hvml`: HVML sample programs.
- `Source/Tools/`: The tools (executables), i.e., the command line programs.
- `Source/Tools/purc`: The standalone HVML interpreter/debugger based-on PurC, which is an interactive command line program.
- `Documents/`: Some documents for developers.

Note that the HTML parser and DOM operations of PurC are derived from:

 - [Lexbor](https://github.com/lexbor/lexbor), which is licensed under the Apache License, Version 2.0.

### TODO List

1. HVML 1.0 Features not implemented yet.
1. HVML 1.0 Predefined Variables not implemented yet.
1. More tests or test cases.
1. More samples.
1. Port PurC to Windows.

For detailed TODO list, please see [TODO List](TODO.md).

### Other documents

For the release notes, please refer to [Release Notes](RELEASE-NOTES.md).

For the community conduct, please refer to [Code of Conduct](Documents/CODE_OF_CONDUCT.md).

For the coding convention, please refer to [Coding Convention](Documents/CODING_CONVENTION.md).

## Authors and Contributors

- Vincent Wei: The architect.
- XUE Shuming: A key developer, the maintainer of most modules and PurC Fetcher.
- XU Xiaohong: A key developer, who implemented the most features of variant and most features of HVML interperter.
- LIU Xin: A developer, the maintainer of the external dynamic variant object `FILE`.
- GENG Yue: A commiter, who implemented some built-in dynamic variant objects.

## Copying

### PurC

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

### ExtDVObjs/fs

Copyright (C) 2022 LIU Xin

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

### purc

Copyright (C) 2022 [FMSoft Technologies]

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

## Tradmarks

1) `HVML` is a registered tradmark of [FMSoft Technologies] in China and other contries or regions.

![HVML](https://www.fmsoft.cn/application/files/8116/1931/8777/HVML256132.jpg)

2) `呼噜猫` is a registered tradmark of [FMSoft Technologies] in China and other contries or regions.

![呼噜猫](https://www.fmsoft.cn/application/files/8416/1931/8781/256132.jpg)

3) `Purring Cat` is a tradmark of [FMSoft Technologies] in China and other contries or regions.

![Purring Cat](https://www.fmsoft.cn/application/files/2816/1931/9258/PurringCat256132.jpg)

4) `PurC` is a tradmark of [FMSoft Technologies] in China and other contries or regions.

![PurC](https://www.fmsoft.cn/application/files/5716/2813/0470/PurC256132.jpg)

5) `xGUI` is a tradmark of [FMSoft Technologies] in China and other contries or regions.

![xGUI](https://www.fmsoft.cn/application/files/cache/thumbnails/7fbcb150d7d0747e702fd2d63f20017e.jpg)

[Beijing FMSoft Technologies Co., Ltd.]: https://www.fmsoft.cn
[FMSoft Technologies]: https://www.fmsoft.cn
[FMSoft]: https://www.fmsoft.cn
[HybridOS Official Site]: https://hybridos.fmsoft.cn
[HybridOS]: https://hybridos.fmsoft.cn

[HVML]: https://github.com/HVML
[Vincent Wei]: https://github.com/VincentWei
[MiniGUI]: https://github.com/VincentWei/minigui
[WebKit]: https://webkit.org
[HTML 5.3]: https://www.w3.org/TR/html53/
[DOM Specification]: https://dom.spec.whatwg.org/
[WebIDL Specification]: https://heycam.github.io/webidl/
[CSS 2.2]: https://www.w3.org/TR/CSS22/
[CSS Box Model Module Level 3]: https://www.w3.org/TR/css-box-3/

[React.js]: https://reactjs.org
[Vue.js]: https://vuejs.org

[HVML Specifiction V1.0]: https://github.com/HVML/hvml-docs/blob/master/zh/hvml-spec-v1.0-zh.md
[HVML Predefined Variables V1.0]: https://github.com/HVML/hvml-docs/blob/master/zh/hvml-spec-predefined-variables-v1.0-zh.md

