[中文版](README-zh.md)

# PurC

`PurC` is the prime HVML interpreter for C/C++ language.

**Table of Contents**

[//]:# (START OF TOC)

- [Introduction](#introduction)
- [Building PurC](#building-purc)
- [Using `purc`](#using-purc)
- [Contributing](#contributing)
- [Authors and Contributors](#authors-and-contributors)
- [Copying](#copying)
- [Tradmarks](#tradmarks)

[//]:# (END OF TOC)

## Introduction

`HVML` is a descriptive programming language proposed and designed by [Vincent Wei],
    who is the author of [MiniGUI], one of the earliest open-source software projects in China.

`PurC` is the acronym of `the Prime HVML inteRpreter for C/C++ language`.
It is also the abbreviation of `Purring Cat`,
   while `Purring Cat` is the nickname and the mascot of HVML.

The goal of PurC is to implement all features defined by [HVML Specifiction V1.0]
and all predefined dynamic objects defined by [HVML Predefined Variables V1.0] in C language.

You can use PurC to run an HVML program or an HVML app by using the command line tool `purc`,
    or use PurC as a library to build your own HVML interpreter.

We release the PurC library under LGPLv3, so it is free for commercial use if you follow the conditions and terms of LGPLv3.

This is version 0.9.7 of PurC.
By now, PurC provides support for Linux and macOS.
The support for Windows is on the way.
We welcome anyone to port PurC to other platforms.

To learn the basic concepts of HVML programming, please refer to the following tutorial:

- [Learn HVML Programming in 30 Minutes](https://github.com/HVML/hvml-docs/blob/master/en/learn-hvml-programming-in-30-minutes-en.md)

For more information about HVML, please refer to the article (10% complete):

- [HVML, a Programable Markup Language](https://github.com/HVML/hvml-docs/blob/master/en/an-introduction-to-hvml-en.md)

For documents, specifications, and open-source software related to HVML, please refer to the following repositories:

- HVML Documents: <https://github.com/HVML/hvml-docs>.
- PurC (the Prime hVml inteRpreter for C language): <https://github.com/HVML/PurC>.
- xGUI Pro (an advanced HVML renderer based on WebKit): <https://github.com/HVML/xGUI-Pro>.
- PurC Midnight Commander (an HVML renderer in text mode): <https://github.com/HVML/PurC-Midnight-Commander>.

Note that, since PurC 0.9.0, we merged the repositories of DOM Ruler and PurC Fetcher to this repository.
Therefore, the following repositories were marked deprecated:

- PurC Fetcher (the remote data fetcher for PurC).
- DOM Ruler (A library to maintain a DOM tree, layout, and stylize the DOM elements by using CSS).

## Building PurC

Note that, if you are seeking the pre-built packages for platforms such as Ubuntu, Deepin, Homebrew, and MSYS2, you can refer to the following page:

<https://hvml.fmsoft.cn/software>

### Prerequisites

To build PurC from source code, please make sure that the following tools or libraries are available on your Linux or macOS system:

1. The cross-platform build system generator: CMake 3.15 or later
2. A C11 and CXX17 compliant compiler: GCC 8+ or Clang 6+
3. Zlib 1.2.0 or later
4. Glib 2.44.0 or later
6. BISON 3.0 or later
7. FLEX 2.6.4 or later
5. Python 3 (Python 3.9.0 or later if you want to build the external dynamic variant object `$PY` to use Python in HVML).
8. Ncurses 5.0 or later (optional; needed by Foil renderer in `purc`)

Although the port for Windows is still on the way, it is possible to build PurC on Windows 10 version 2004 or later:
You can install WSL (Windows Subsystem for Linux) and a Linux distribution, e.g., Ubuntu, on your Windows system,
    then build PurC in the Ubuntu environment.

### Building steps

We assume that you are using Linux.

After fetching the source of PurC, you can change to the root of the source tree,
      and use the following command line to build and install PurC:

```
$ cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPORT=Linux -B build && cmake --build build && sudo cmake --install build
```

The above command line consists of the following commands:

1. `cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPORT=Linux -B build`: Change to the `build/`
subdirectory and run `cmake` to generate the building files to build PurC.
Note that this command uses the following options:
   - `-DCMAKE_BUILD_TYPE=RelWithDebInfo`: Specify the building type as `RelWithDebInfo`.
   You can also use `Debug`, `Release`, and other options supported by `cmake`.
   - `-DPORT=Linux`: Tell `cmake` that you are building PurC for an operating system
   based on the Linux kernel. Use `-DPORT=Mac` if you are using macOS.
   - `-B build`: Generate building files in the `build/` subdirectory.
1. `cmake --build build`: Build PurC in the `build/` subdirectory.
1. `sudo cmake --install build`: Install PurC from the `build/` subdirectory.

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
   you can use the following commands:

```bash
$ cd <path/to/the/root/of/the/source/tree/of/PurC>
$ cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPORT=Linux -Bbuild -GNinja &&  ninja -Cbuild && sudo ninja -Cbuild install
```

Note that you might need to remove the `build/` directory first if there is already one.

By default, the above commands will build PurC and install the headers, libraries, executables,
   and some documents to your system (under the `/usr/local/` directory if you are using a Linux system).

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
- `PURC_USER_DIR_SUFFIX`: The directory suffix for the user.
- `PURC_LOG_ENABLE`: `true` if enabling the global log facility.
- `PURC_LOG_SYSLOG`: `true` if enabling to use `syslog` as the log facility.

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

### Run a HVML program with errors or exceptions

Please save the following contents in a file named `error.hvml` in your working directory:

```hvml
<!DOCTYPE hvml>
<hvml target="void">

    $STREAM.stdout.writelines('Hello, world!)

</hvml>
```

We missed the second single quote of `Hello, world!` in the code above.
The interpreter will exit with a nonzero return value if you run `purc` without any options:

```bash
$ purc error.hvml
$ echo $?
1
```

You can run `purc` with the option `-v` for a verbose message:

```bash
$ purc -v error.hvml
purc 0.9.7
Copyright (C) 2022, 2023 FMSoft Technologies.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Failed to load HVML from file:///srv/devel/hvml/purc/build/error.hvml: pcejson unexpected eof parse error
Parse file:///srv/devel/hvml/purc/build/error.hvml failed : line=7, column=1, character=0x0
```

This time, `purc` reported the error it encountered when it was parsing the HVML program: the wrong line and column.

If you change the program to add the missing single quote, `purc` will be happy to execute the HVML program.

For an uncaught runtime exception, `purc` will dump the executing stack.

For example, you can save the following program as `exception.hvml`:

```hvml
<!DOCTYPE hvml>
<hvml target="void">
    <iterate on 0 onlyif $L.lt($0<, 10) with $DATA.arith('+', $0<, 1) nosetotail >
        $STREAM.stdout.writelines("$0<) Hello, world! $CRTN.foo")
    </iterate>
</hvml>
```

This HVML program refers to an inexistent property (`foo`) of `$CRTN`.

Run `purc` to execute this HVML program with `-b` option, it will report the executing stack:

```
$ purc -v exception.hvml
purc 0.9.7
Copyright (C) 2022 FMSoft Technologies.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

Executing HVML program from `file:///srv/devel/hvml/purc/build/exception.hvml`...

The main coroutine terminated due to an uncaught exception: NoSuchKey.
>> The document generated:

>> The executing stack frame(s):
#00: <iterate on=0 onlyif=$L.lt( $0<, 10 ) with=$DATA.arith( "+", $0<, 1 ) nosetotail>
  ATTRIBUTES:
    on: 0
    onlyif: true
    with: 1L
  CONTENT: `NoSuckKey` raised when evaluating the expression: $STREAM.stdout.writelines( "$0<) Hello, world! $CRTN.foo" )
    Variant Creation Model: callGetter(getElement(getElement(getVariable("STREAM"),"stdout"),"writelines"),concatString(getVariable("0<"),") Hello, world! ",getElement(getVariable("CRTN"),"foo")))
    Call stack:
      #00: $CRTN.foo
        Variant Creation Model: getElement(getVariable("CRTN"),"foo")
      #01: "$0<) Hello, world! $CRTN.foo"
        Variant Creation Model: concatString(getVariable("0<"),") Hello, world! ",getElement(getVariable("CRTN"),"foo"))
      #02: $STREAM.stdout.writelines( "$0<) Hello, world! $CRTN.foo" )
        Variant Creation Model: callGetter(getElement(getElement(getVariable("STREAM"),"stdout"),"writelines"),concatString(getVariable("0<"),") Hello, world! ",getElement(getVariable("CRTN"),"foo")))
  CONTEXT VARIABLES:
    < 0
    @ null
    ! {}
    : null
    = null
    % 0UL
    ^ null
#01: <hvml target="void">
  ATTRIBUTES:
    target: "void"
  CONTENT: undefined
  CONTEXT VARIABLES:
    < null
    @ null
    ! {}
    : null
    = null
    % 0UL
    ^ null
```

### Run multiple HVML programs in parallel

PurC can run multiple HVML programs as coroutines in parallel.

For example, we enhance the first HVML program to print `Hello, world!` 10 times:

```hvml
<!DOCTYPE hvml>
<hvml target="void">
    <iterate on 0 onlyif $L.lt($0<, 10) with $DATA.arith('+', $0<, 1) nosetotail must-yield >
        $STREAM.stdout.writelines(
                $STR.join($0<, ") Hello, world! --from COROUTINE-", $CRTN.cid))
    </iterate>
</hvml>
```

Assume you named the enhanced version as `hello-10.hvml`,
       we can run the program as two coroutines in parallel by specifying the command line flag `-l`:

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

One of the important differences between HVML and other programming languages is that HVML can generate documents described in markup languages like HTML,
    not just output data to a file or your terminal.

For your convenience, we have prepared some HVML samples in the directory `Source/Samples/hvml` of this repository.
After building PurC, the samples will be copied to the building root directory, under the `hvml/` subdirectory,
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
This renderer will record the messages sent by PurC to the renderer in a local file,
     it is `/dev/null` by default on Linux.
Because this HVML program did not use `$STREM.stdout` anymore, you will see nothing on your terminal.
But you can use the option `--verbose` (or the short option `-v`) to show the HTML contents generated by the HVML program in your terminal:

```bash
$ purc -v hvml/fibonacci-html-temp.hvml
```

The command will give you the following output:

```
purc 0.9.7
Copyright (C) 2022, 2023 FMSoft Technologies.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

Executing HVML program from `file:///srv/devel/hvml/purc/build/hvml/fibonacci-html-temp.hvml`...

The main coroutine exited.
>> The document generated:

<html>
  <head>
    <title>Fibonacci Numbers
    </title>
  </head>
  <body id="theBody">
    <h1>Fibonacci Numbers less than 2000
    </h1>
    <ol>
      <li>0
      </li>
      <li>1
      </li>
      <li>1
      </li>
      <li>2
      </li>
      <li>3
      </li>
      <li>5
      </li>
      <li>8
      </li>
      <li>13
      </li>
      <li>21
      </li>
      <li>34
      </li>
      <li>55
      </li>
      <li>89
      </li>
      <li>144
      </li>
      <li>233
      </li>
      <li>377
      </li>
      <li>610
      </li>
      <li>987
      </li>
      <li>1597
      </li>
    </ol>
    <p>Totally 18 numbers
    </p>
  </body>
</html>

>> The executed result:
[ 18, 1597 ]
```

`purc` now contains a built-in renderer called `Foil`,
    which can show the contents of the target document in your terminal according to CSS properties.
We can use the option `--rdr-comm=thread` (`-c thread` for short) when running `purc` to use the renderer:

```bash
$ purc -c thread hvml/fibonacci-html-temp.hvml
```

Here is the screenshot on macOS:

![Fibonacci Numbers in Foil](https://files.fmsoft.cn/hvml/screenshots/fibonacci-html-temp-foil.png)

You can also try other samples which illustrate the features of the Foil renderer:

- `hvml/foil-layouts.hvml`
- `hvml/foil-progress.hvml`
- `hvml/foil-meter.hvml`

Note that in the current version (0.9.7), Foil is not fully functional.
Shortly, Foil will provide support for most properties of CSS 2.2 and some properties of CSS Level 3,
   so that you can get a similar experience to a web browser.

You can also direct `purc` to connect to a graphics renderer, for example, `xGUI Pro`.
It is an advanced HVML renderer based on WebKit.

Assume that you have installed xGUI Pro on your system,
       you can run `purc` to show the ultimate HTML contents in a window of xGUI Pro.
Please refer to <https://github.com/HVML/xGUI-Pro> for detailed instructions to install xGUI Pro.

Assume that you have started xGUI Pro from another terminal, then please run `purc` with the following options:

```bash
$ purc --rdr-comm=socket hvml/fibonacci-html-temp-rdr.hvml
```

Note that, in the above command line, we execute a modified version of Fibonacci Numbers: `hvml/fibonacci-html-temp-rdr.hvml`.
If you compare these two versions, you will find that there is an `observe` element in the modified version:

```hvml
        <observe on $CRTN for "rdrState:pageClosed">
            <exit with [$count, $last_two] />
        </observe>
```

If there is no such `observe` element, the HVML program will exit immediately after generating the HTML document.
By using the `observe` element, the HVML program will wait for the time when the page created by the renderer is closed by the user (that is you).

You will see that the contents in a window of xGUI Pro created by `hvml/fibonacci-html-temp-rdr.hvml`:

![Fibonacci Numbers](https://files.fmsoft.cn/hvml/screenshots/fibonacci-html-temp.png)

If you close the window by clicking the close box on the caption bar,
   the HVML program will exit as normal.

For a complete HVML program that gives a better experience,
    you can try to run another sample called `hvml/calculator-bc.hvml`, which implements an arbitrary precision calculator:

```bash
$ purc -c socket hvml/calculator-bc.hvml
```

Here is the screenshot of `hvml/calculator-bc.hvml`:

![the Arbitrary Precision Calculator](https://files.fmsoft.cn/hvml/screenshots/calculator-bc.png)

Or run `hvml/planetary-resonance-lines.hvml`, which shows the Planetary Resonance:

```bash
$ purc -c socket hvml/planetary-resonance-lines.hvml
```

Here is the screenshot of `hvml/planetary-resonance-lines.hvml`:

![the Planetary Resonance](https://files.fmsoft.cn/hvml/screenshots/planetary-resonance.png)

For an amazing HVML program that uses multiple coroutines to sieve the prime numbers,
    you can run `hvml/prime-number-sieve.hvml`, which visually illustrates the prime number sieve algorithm:

```bash
$ purc -c socket hvml/prime-number-sieve.hvml
```

Here is the screenshot of `hvml/prime-number-sieve.hvml`:

![the Prime Number Sieve](https://files.fmsoft.cn/hvml/screenshots/prime-number-sieve.png)

For example, to use an external dynamic object defined in a shared library in an HVML program,
    you can run `hvml/file-manager.hvml`, which illustrates the usage of the external dynamic object `$FS`:

```
$ purc -c socket hvml/file-manager.hvml
```

Below is the screenshot of `hvml/file-manager.hvml`:

![the File Manager](https://files.fmsoft.cn/hvml/screenshots/file-manager.png)

### Options for `purc`

You can see the all options supported by `purc` when you run `purc` with `-h` option:

```bash
$ purc -h
purc (0.9.7) - a standalone HVML interpreter/debugger based on PurC.
Copyright (C) 2022, 2023 FMSoft Technologies.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

Usage: purc [ options ... ] [ file | url ] ... | [ app_desc_json | app_desc_ejson ]

The following options can be supplied to the command:

  -a --app=< app_name >
        Run with the specified app name (the default value is `cn.fmsoft.hvml.purc`).

  -r --runner=< runner_name >
        Run with the specified runner name (the default value is `main`).

  -d --data-fetcher=< local | remote >
        The data fetcher; uses `local` or `remote`.
            - `local`: the built-in data fetcher, and only `file://` URLs
               supported.
            - `remote`: the remote data fetcher to support more URL schemas,
               such as `http`, `https`, `ftp` and so on.

  -c --rdr-comm=< headless | thread | socket >
        The renderer communication method; uses `headless` (default), `thread`, or `socket`.
            - `headless`: the built-in headless renderer.
            - `thread`: the built-in thread-based renderer.
            - `socket`: the remote socket-based renderer;
              `purc` will connect to the renderer via Unix Socket or WebSocket.
  -u --rdr-uri=< renderer_uri >
        The renderer uri:
            - For the renderer comm method `headless`,
              the default value is `file:///dev/null`.
            - For the renderer comm method `thread`,
              the default value is `edpt://localhost/cn.fmsoft.hvml.renderer/foil`.
            - For the renderer comm method `socket`,
              the default value is `unix:///var/tmp/purcmc.sock`.

  -j --request=< json_file | - >
        The JSON file contains the request data which will be passed to
        the HVML programs; use `-` if the JSON data will be given through
        STDIN stream. (Ctrl+D for end of input if you input the JSON data in a terminal.)

  -q --query=< query_string >
        Use a URL query string (in RFC 3986) for the request data which will be passed to
        the HVML programs; e.g., --query='case=displayBlock&lang=zh'.

  -l --parallel
        Execute multiple programs in parallel.

  -v --verbose
        Execute the program(s) with verbose output.

  -C --copying
        Display detailed copying information and exit.

  -V --version
        Display version information and exit.

  -h --help
        This help.
```

### Run an HVML app in multiple runners

PurC supports running an app within multiple runners.
Here one `runner` is one thread in the `purc` process.

For this purpose, you can prepare a JSON file or an eJSON file that defines the app, the runners,
    and the initial HVML programs to run as coroutines in different runners.

Here is an example:

```json
{
    "app": "cn.fmsoft.hvml.sample",
    "runners": [
        {
            "runner": "Products",
            "renderer": { "protocol": "socket", "uri": "unix:///var/tmp/purcmc.sock",
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
            "renderer": { "protocol": "socket", "uri": "unix:///var/tmp/purcmc.sock",
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

For example, we can specify the command line options:

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
            "renderer": { "protocol": "socket", "uri": "unix:///var/tmp/purcmc.sock",
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

All occurrences of `$OPTS.app` in `my_app.ejson` will be substituted by `cn.fmsoft.hvml.sample`.

### More HVML samples

You can find more HVML sample programs in the repository [HVML Documents](https://github.com/HVML/hvml-docs),
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
Since 0.9.0, PurC Fetcher had been included in this repository, and it will be built along with PurC.

## Contributing

We welcome anybody to take part in the development and contribute your effort!
There are many ways to contribute to PurC:

- Participate in Q&A in our [GitHub Discussions](https://github.com/HVML/PurC/discussions).
- [Submit bugs](https://github.com/HVML/PurC/issues) and help us verify fixes as they are checked in.
- Review the [source code changes](https://github.com/HVML/PurC/pulls).
- Contribute bug fixes.
- Contribute test programs and/or test cases.
- Contribute samples (HVML samples or C/C++ sample programs to use PurC API).

### Current Status

This project was launched in June. 2021, and we opened this repo in July 2022.
This is version 0.9.7 of PurC.

The main purpose of PurC is to provide a library for you to write your own HVML interpreter.
The current version implements almost all features defined by [HVML Specification V1.0],
      and also implements almost all predefined dynamic variables defined by [HVML Predefined Variables V1.0].
We plan to release PurC version 1.0 in June 2023.

Except for the HVML interpreter, PurC also provides many fundamental features for general C programs:

1. PurC provides the APIs for variant management, here variant is the way HVML manages data.
1. PurC provides the APIs for parsing JSON and extended JSON.
1. PurC provides the APIs for parsing and evaluating a parameterized eJSON expression.
1. PurC provides the APIs for parsing an HTML document.
1. PurC provides the APIs for creating multiple HVML runners.
1. PurC provides the APIs for parsing an HVML program and scheduling to run it.

You can use these groups of APIs independently according to your needs.

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
- `Source/PurC/dom/`: The implementation of the DOM tree.
- `Source/PurC/vdom/`: The implementation of the virtual DOM tree.
- `Source/PurC/html/`: The implementation of the HTML parser. The HTML parser reads an HTML document or document fragments and constructs an eDOM tree.
- `Source/PurC/hvml/`: The implementation of the HVML parser. The HTML parser reads an HVML document and constructs a vDOM tree.
- `Source/PurC/xgml/`: The implementation of the XGML parser (Not implemented so far). The XGML parser reads an XGML document or document fragments and constructs an eDOM tree.
- `Source/PurC/xml/`: The XML parser (Not implemented so far). The XML parser parses an XML document or document fragments and constructs an eDOM tree.
- `Source/PurC/instance/`: The operations of PurC instances and sessions.
- `Source/PurC/fetchers/`: The data fetchers fetch data from various data sources (FILE, HTTP, FTP, and so on).
- `Source/PurC/executors/`: The implementation of internal executors.
- `Source/PurC/interpreter/`: The vDOM interpreter.
- `Source/PurC/pcrdr/`: The management of the connection to the renderer.
- `Source/PurC/ports/`: The ports for different operating systems, such as a POSIX-compliant system or Windows.
- `Source/PurC/bindings/`: The bindings for Python, Lua, and other programming languages.
- `Source/ExtDVObjs/math/`: The implementation of the external dynamic variant object `$MATH`.
- `Source/ExtDVObjs/fs/`: The implementation of the external dynamic variant object `$FS` and `$FILE`.
- `Source/CSSEng/`: The CSS parsing and selecting engine derived from libcss of NetSurf project.
- `Source/DOMRuler/`: The library to lay out and stylize a DOM tree by using CSSEng.
- `Source/RemoteFetcher/`: The library used by the PurC Remote Fetcher.
- `Source/WTF/`: The simplified WTF (Web Template Framework) from WebKit.
- `Source/cmake/`: The cmake modules.
- `Source/ThirdParty/`: The third-party libraries, such as `gtest`.
- `Source/test/`: The unit test programs.
- `Source/Samples/api`: Samples for using the API of PurC.
- `Source/Samples/hvml`: HVML sample programs.
- `Source/Executables/`: The executables.
- `Source/Executables/purc`: The standalone HVML interpreter/debugger based on PurC, which is an interactive command line program.
- `Source/Executables/purc-fetcher`: The ultimate executable of PurC Remote Fetcher.
- `Source/Tools/`: The tools or scripts for maintaining this project.
- `Source/Tools/aur`: AUR package packaging scripts.
- `Source/Tools/debian`: DEB package packaging scripts.
- `Documents/`: Some documents for developers.

Note that

1. The source code in `Source/WTF` is derived from [WebKit](https://www.webkit.org/), which is licensed under BSD and LGPLv2.1.
1. The source code in `Source/CSSEng` is derived from LibCSS, LibParserUtils, and LibWapcaplet of [NetSurf project](https://www.netsurf-browser.org/). These three libraries are licensed under MIT.
1. The source code in `Source/RemoteFetcher` is derived from [WebKit](https://www.webkit.org/), which is licensed under BSD and LGPLv2.1.
1. The HTML parser and DOM operations of PurC are derived from [Lexbor](https://github.com/lexbor/lexbor), which is licensed under the Apache License, Version 2.0.

### TODO List

1. HVML 1.0 Features not implemented yet.
1. HVML 1.0 Predefined Variables not implemented yet.
1. More tests or test cases.
1. More samples.
1. Port PurC to Windows.

For the detailed TODO list, please see [TODO List](Documents/TODO.md).

### Other documents

For the release notes, please refer to [Release Notes](RELEASE-NOTES.md).

For community conduct, please refer to [Code of Conduct](Documents/CODE_OF_CONDUCT.md).

For the coding convention, please refer to [Coding Convention](Documents/CODING_CONVENTION.md).

## Authors and Contributors

- Vincent Wei: The architect.
- XUE Shuming: A key developer, the maintainer of most modules, and PurC Fetcher.
- LIU Xin: A developer, the maintainer of the external dynamic variant object `FILE`.
- XU Xiaohong: An early developer, who implemented most features of the variant module and most features of the HVML interperter.
- GENG Yue: An early developer, who implemented some built-in dynamic variant objects.

## Copying

### PurC

Copyright (C) 2021 ~ 2023 [FMSoft Technologies]

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

### CSSEng

CSSEng is derived from LibCSS, LibParserUtils, and LibWapcaplet of [NetSurf project](https://www.netsurf-browser.org/).

These libraries are all licensed under MIT License.

Copyright 2007 ~ 2020 developers of NetSurf project.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
           to deal in the Software without restriction, including without limitation the rights to use,
           copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
           and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
    INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

The new source files developed by FMSoft are licensed under LGPLv3:

Copyright (C) 2021 ~ 2023 [FMSoft Technologies]

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

### DOMRuler

Copyright (C) 2021 ~ 2023 FMSoft <https://www.fmsoft.cn>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General License for more details.

You should have received a copy of the GNU Lesser General License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

### ExtDVObjs/fs

Copyright (C) 2022 LIU Xin
Copyright (C) 2022 [FMSoft Technologies]

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

Copyright (C) 2022, 2023 [FMSoft Technologies]

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

### purc-fetcher

Copyright (C) 2022, 2023 [FMSoft Technologies]

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

