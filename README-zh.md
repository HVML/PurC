# PurC

PurC：

**目录**

[//]:# (START OF TOC)

- [介绍](#介绍)
- [构建 PurC](#构建-purc)
- [使用 purc](#使用-purc)
- [Hacking PurC](#hacking-purc)
- [作者和贡献者](#作者和贡献者)
- [版权信息](#版权信息)
- [商标](#商标)

[//]:# (END OF TOC)

## 介绍

HVML 是由中国首款开源基础软件 [MiniGUI] 的作者[魏永明]设计的描述式编程语言，也是世界首款可编程标记语言。

PurC 是 `the Prime HVML inteRpreter for C language` 的首字母缩写。同时它也可以理解为 `Purring Cat` 的缩写，而 `Purring Cat` 是 HVML 的昵称和吉祥物。

PurC 的目标是使用 C 语言实现 [HVML 规范 V1.0](https://github.com/HVML/hvml-docs/blob/master/zh/hvml-spec-v1.0-zh.md) 中定义的所有功能以及实现 [HVML 预定义变量 V1.0](https://github.com/HVML/hvml-docs/blob/master/zh/hvml-spec-predefined-variables-v1.0-zh.md) 中定义的所有预定义动态对象。

本项目编译后会产生PurC函数库，命令行工具purc，和相关的示例测试等工具。您可以使用命令行工具 `purc` 运行 HVML 程序或 HVML 应用程序，或使用 PurC 作为函数库来构建自己的 HVML 解释器。

我们在 LGPLv3 下发布 PurC，因此，如果您遵循 LGPLv3 的条件和条款，它可以免费用于商业用途。

这是 PurC 的 0.9.2 版本。到目前为止，PurC 为 Linux 和 macOS 提供支持。对 Windows 的支持即将到来。我们欢迎任何人将 PurC 移植到其他平台。

要了解有关 HVML 编程的基本概念，请参考以下教程：

[在 30 分钟内学习 HVML 编程](https://github.com/HVML/hvml-docs/blob/master/en/learn-hvml-programming-in-30-minutes-en.md)

有关 HVML 的更多信息，请参考文章（10% 完成）：

[HVML，一种可编程标记语言](https://github.com/HVML/hvml-docs/blob/master/en/an-introduction-to-hvml-en.md)

有关与 HVML 相关的文档、规格和开源软件，请参考以下代码仓库：

- HVML文档：<https://github.com/HVML/hvml-docs>。
- PurC（HVML 解释器）：<https://github.com/HVML/PurC>。
- xGUI Pro（基于 WebKit 的高级的 HVML 渲染器）：<https://github.com/HVML/xGUI-Pro>。
- PurC Midnight Commander（HVML 字符渲染器）：<https://github.com/HVML/PurC-Midnight-Commander>。

请注意，自 PurC 0.9.0 以来，我们将 DOM Ruler 和 PurC Fetcher 的代码仓库合并到此仓库中。因此，以下仓库被标记为不建议使用：

- PurC Fetcher（PurC 的远程数据获取器）。
- DOM Ruler（使用 CSS 维护 DOM 树、布局和风格化 DOM 元素的函数库）。

## 构建 PurC

请注意，如果您正在为 Ubuntu、Deepin、Homebrew 和 MSYS2 等平台寻找预建软件包，可以参考以下页面：

<https://hvml.fmsoft.cn/software>

### 准备

要从源代码构建 PurC，请确保您的 Linux 或 macOS 系统上提供以下工具或函数库：

1. cmake
2. 兼容 C11 和 CXX17 的编译器:GCC 8+ 或 Clang 6+
3. Zlib 1.2.0 或更高版本
4. Glib 2.44.0 或更高版本
5. Python 3
6. BISON 3.0 或更高版本
7. FLEX 2.6.4 或更高版本

虽然针对 Windows 的移植仍在进行中，但可以在 Windows 10 2004 或更高版本上构建 PurC：您可以在 Windows 系统上安装 WSL（适用于 Linux 的 Windows 子系统）和 Linux 发行版，例如 Ubuntu，然后在 Ubuntu 环境中构建 PurC。

### 构建步骤

我们假设您正在使用 Linux。

在获取了 PurC 源代码之后，您可以切换到源代码树的根目录，使用下面的命令行来构建和安装 PurC:

```
$ cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPORT=Linux -B build && cmake --build build && sudo cmake --install build
```

上述命令行由以下命令组成：

1. `cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPORT=Linux -B build`：更改 `build/` 子目录，并运行 `cmake` 生成构建文件以构建 PurC。请注意，此命令使用以下选项：
   - `-DCMAKE_BUILD_TYPE=RelWithDebInfo`：指定构建类型为`RelWithDebInfo`。您还可以使用 `Debug`、`Release` 和 `cmake` 支持的其他选项。
   - `-DPORT=Linux`：告诉 `cmake` 您正在为基于 Linux 内核的操作系统构建 PurC。如果您使用 macOS，请使用 `-DPORT=Mac`。
   - `-B build`：在 `build/` 子目录中生成构建文件。

2. `cmake --build build`：在 `build/` 子目录中构建 PurC。
3. `sudo cmake --install build`：从 `build/` 子目录安装 PurC。

您还可以使用以下命令逐步构建和安装 PurC：

```bash
$ cd <path/to/the/root/of/the/source/tree/of/PurC>
$ cd
$ mkdir build/
$ cd build/
$ cmake -DCMAKE_BUILD_TYPE=Release -DPORT=Linux ..
$ make -j4
$ sudo make install
```

如果您想使用 `ninja` 而不是 `make` 来构建 PurC，可以使用以下命令：

```bash
$ cd <path/to/the/root/of/the/source/tree/of/PurC>
$ cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPORT=Linux -Bbuild -GNinja &&  ninja -Cbuild && sudo ninja -Cbuild install
```

请注意，如果已经存在 `build/` 目录，您可能需要先删除它。

默认情况下，上述命令将构建 PurC，并将头文件、库、可执行文件和一些文档安装到您的系统（如果您使用的是 Linux 系统，则在 `/usr/local/` 目录下）。

使用 `make` 时，您可以使用 `DESTDIR` 指定替代安装目录：

```bash
$ make DESTDIR=/package/stage install
```

使用 `ninja` 时，您也可以使用 `DESTDIR` 指定替代安装目录：

```bash
$ DESTDIR="/package/stage" ninja -Cbuild install
```

## 环境变量

PurC 将以下环境变量用于不同的目的：

- `PURC_DVOBJS_PATH`：为外部动态对象保存共享模块的路径。
- `PURC_EXECUTOR_PATH`：为外部执行者保存共享模块的路径。
- `PURC_FETCHER_EXEC_PATH`：保存 PurC Fetcher 可执行程序的路径。
- `PURC_USER_DIR_SUFFIX`：用户的目录后缀。
- `PURC_LOG_ENABLE`：如果启用全局日志工具，则为 `true`。
- `PURC_LOG_SYSLOG`：如果启用使用 syslog 作为日志工具，则为 `true` 。

## 使用 PurC

以下部分假设您已将 PurC 安装到您的系统，并且命令行工具 `purc` 已安装到 `/usr/local/bin/`。确保您已将 `/usr/local/lib` 添加到 `/etc/ld.so.conf` 并运行 `sudo ldconfig` 命令，以便系统可以找到您刚刚安装到 `/usr/local/lib` 中的 PurC 函数库。

### 运行单个 HVML 程序

请将以下内容保存在名为 `hello.hvml` 的文件中，作为您工作目录中的第一个 HVML 程序：

```hvml
<!DOCTYPE hvml>
<hvml target="void">

    $STREAM.stdout.writelines('Hello, world!')

</hvml>
```

要运行此 HVML 程序，您可以通过以下方式使用 `purc`：

```bash
$ purc hello.hvml
```

您会看到第一个 HVML 程序在终端上打印 `Hello, world!` 并退出：

```
Hello, world!
```

如果您在 HVML 程序的第一行预先添加下面这行命令，也可以直接以脚本运行此 HVML 程序：

```
#!/usr/local/bin/purc
```

完成后，运行以下命令，更改文件的模式以获得执行权限：

```bash
$ chmod +x hello.hvml
```

然后直接从命令行运行 `hello.hvml`：

```bash
$ ./hello.hvml
```

### 运行错误或异常的 HVML 程序

请将以下内容保存在工作目录中名为 `error.hvml` 的文件中：

```hvml
<!DOCTYPE hvml>
<hvml target="void">

    $STREAM.stdout.writelines('Hello, world!)

</hvml>
```

显然，我们忘记了 `Hello, world!` 的第二个单引号。如果在没有任何选项的情况下运行 `purc`，解释器将以一个非零返回值退出：

```bash
$ purc error.hvml
$ echo $?
1
```
您想获得详细信息，可以使用 `-v` 选项运行 `purc`：

```bash
$ purc -v error.hvml
purc 0.9.2
Copyright (C) 2022 FMSoft Technologies.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Failed to load HVML from file:///srv/devel/hvml/purc/build/error.hvml: pcejson unexpected eof parse error
Parse file:///srv/devel/hvml/purc/build/error.hvml failed : line=7, column=1, character=0x0
```

这时，`purc` 报告了它在解析 HVML 程序时遇到的错误：错误的行和列。

如果您更改程序以添加缺失的单引号，`purc` 将很乐意执行 HVML 程序。

对于未捕获的运行时异常，`purc` 将转储正在执行的栈。。

例如，您可以将以下程序保存为 `exception.hvml`：

```hvml
<!DOCTYPE hvml>
<hvml target="void">
    <iterate on 0 onlyif $L.lt($0<, 10) with $DATA.arith('+', $0<, 1) nosetotail >
        $STREAM.stdout.writelines("$0<) Hello, world! $CRTN.foo")
    </iterate>
</hvml>
```

这段 HVML 程序引用了默认变量 `$CRTN` 中一个并不存在的属性 `foo`。

使用 `-b` 选项运行 `purc` 来执行这个 HVML 程序，它将报告正在执行的栈：

```
$ purc -v exception.hvml
purc 0.9.2
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
  CONTENT: `NoSuckKey` raised when evaluating the experssion: $STREAM.stdout.writelines( "$0<) Hello, world! $CRTN.foo" )
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

### 并行运行多个 HVML 程序

PurC 可以并行运行多个 HVML 程序作为协程。

例如，我们增强第一个 HVML 程序来打印 `Hello, world!` 10 次:

```hvml
<!DOCTYPE hvml>
<hvml target="void">
    <iterate on 0 onlyif $L.lt($0<, 10) with $DATA.arith('+', $0<, 1) nosetotail must-yield >
        $STREAM.stdout.writelines(
                $STR.join($0<, ") Hello, world! --from COROUTINE-", $CRTN.cid))
    </iterate>
</hvml>
```

假设将增强版本命名为 `hello-10.hvml`，我们可以通过指定命令行标志 `-l` 并行运行该程序作为两个协程：

```bash
$ purc -l hello-10.hvml hello-10.hvml
```

您将在终端上看到以下输出：

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

在上面的输出中，`COROUTINE-3` 和 `COROUTINE-4` 表示 PurC 为该程序的两个运行实例分配的协程标识符。您可以看到，PurC 安排运行实例交替执行，即以协程的方式执行。


如果您不使用命令行中的 `-l` 标志，`purc` 将逐个运行这些程序：

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

### 连接到 HVML 渲染器

HVML 程序除了像其他通常的程序一样将运行时的一些结果输出到文件或者终端上以外，还能同时输出 HTML 或者 XML 等标记文档，这些文档可以被渲染器进行渲染从而得到图形化的显示。


*为了方便起见，我们在此存储库的 `Source/Samples/hvml`
目录中准备了一些 HVML 示例。在构建 PurC 之后，示例将被复制到构建根目录，在 `hvml/` 子目录下，这样就可以更改到构建根目录并使用 `purc` 运行示例。例如:*

```bash
$ cd <path/to/the/building/directory/>
$ purc hvml/fibonacci-void-temp.hvml
```

这个 HVML 程序将输出 18 个小于 2000 的斐波那契数字。这个程序还有另一个版本：`hvml/fibonacci-html-temp.hvml`。它将生成一个列出斐波那契数字的 HTML 文档。

如果您使用 `purc` 运行 `hvml/fibonacci-html-temp.hvml` 程序，没有任何选项，`purc` 将使用名为 `HEADLESS` 的渲染器。此渲染器将记录 PurC 发送给渲染器到本地文件的消息，在 Linux 上默认为 `/dev/null`。由于此 HVML 程序不再使用 `$STREM.stdout`，因此在终端上看不到任何东西。但您可以使用选项 `--verbose`（或短选项 `-b`）在终端中显示 HVML 程序生成的 HTML 内容：

```bash
$ purc -v hvml/fibonacci-html-temp.hvml
```

该命令将为您提供以下输出：

```
purc 0.9.2
Copyright (C) 2022 FMSoft Technologies.
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

`purc` 现在包含一个名为 `Foil` 的内置渲染器。此渲染器可以在终端中显示目标文档的内容：

```bash
$ purc --rdr-comm=thread hvml/fibonacci-html-temp.hvml
Fibonacci Numbers less than 2000
1. 0
2. 1
3. 1
4. 2
5. 3
6. 5
7. 8
8. 13
9. 21
10. 34
11. 55
12. 89
13. 144
14. 233
15. 377
16. 610
17. 987
18. 1597
Totally 18 numbers
```

请注意，在当前版本（0.9.2）中，Foil 功能不完全。在不久的将来，Foil 将支持 CSS 2.2 大多数功能和 CSS Level 3 的某些功能，以便您可以获得像网页浏览器一样的类似体验。

您还可以直接将 `purc` 连接到图形渲染器，例如 `xGUI Pro`。`xGUI Pro` 是一种基于 WebKit 的高级 HVML 渲染器。

假设您在系统上安装了 xGUI Pro，可以运行 `purc` 在 xGUI Pro 的窗口中显示最终的 HTML 内容。有关安装 xGUI Pro 的详细说明，请参阅 <https://github.com/HVML/xGUI-Pro>。

假设您已从另一个终端启动 xGUI Pro，那么请使用以下选项运行 `purc`：

```bash
$ purc --rdr-comm=socket hvml/fibonacci-html-temp-rdr.hvml
```

请注意，在上述命令行中，我们执行 Fibonacci Numbers 的修改版本：`hvml/fibonacci-html-temp-rdr.hvml`。如果您比较这两个版本，会发现修改后的版本中有一个 `observe` 元素：

```hvml
        <observe on $CRTN for "rdrState:pageClosed">
            <exit with [$count, $last_two] />
        </observe>
```

如果没有这样的 `observe` 元素，HVML 程序将在生成 HTML 文档后立即退出。通过使用 `observe` 元素，HVML 程序将等待渲染器创建的页面被用户（即您）关闭。

您将看到由 `hvml/fibonacci-html-temp-rdr.hvml` 创建的 xGUI Pro 窗口中的内容：

![fibonacci-html-temp](https://files.fmsoft.cn/hvml/screenshots/fibonacci-html-temp.png)

如果您通过单击标题栏上的关闭框关闭窗口，HTML 程序将正常退出。

对于提供更好体验的完整 HVML 程序，您可以尝试运行另一个名为 `hvml/calculator-bc.hvml` 的示例，该示例实现了任意精度计算器：

```bash
$ purc -p purcmc hvml/calculator-bc.hvml
```

这是 `hvml/calculator-bc.hvml` 的截图：

![the Arbitrary Precision Calculator](https://files.fmsoft.cn/hvml/screenshots/calculator-bc.png)

或者运行 `hvml/planetary-resonance-lines.hvml`，该程序展示了行星共振：

```bash
$ purc -p purcmc hvml/planetary-resonance-lines.hvml
```

以下是 `hvml/planetary-resonance-lines.hvml` 的截图：

![the Planetary Resonance](https://files.fmsoft.cn/hvml/screenshots/planetary-resonance.png)

下面的示例展示了一个使用多个协程筛选素数的 HVML 程序，您可以运行 `hvml/prime-number-sieve.hvml`，这直观地展示了素数筛算法：

```
$ purc -p purcmc hvml/prime-number-sieve.hvml
```

这是 `hvml/prime-number-sieve.hvml` 的截图：

![the Prime Number Sieve](https://files.fmsoft.cn/hvml/screenshots/prime-number-sieve.png)

### 选项 purc

当您使用 `-h` 选项运行 `purc` 时，您可以看到 `purc` 支持的所有选项：

```bash
$ purc -h
purc (0.9.2) - a standalone HVML interpreter/debugger based-on PurC.
Copyright (C) 2022 FMSoft Technologies.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

Usage: purc [ options ... ] [ file | url ] ... | [ app_desc_json | app_desc_ejson ]

The following options can be supplied to the command:

  -a --app=< app_name >
        Run with the specified app name (default value is `cn.fmsoft.hvml.purc`).

  -r --runner=< runner_name >
        Run with the specified runner name (default value is `main`).

  -d --data-fetcher=< local | remote >
        The data fetcher; use `local` or `remote`.
            - `local`: use the built-in data fetcher, and only `file://` URLs
               supported.
            - `remote`: use the remote data fetcher to support more URL schemas,
               such as `http`, `https`, `ftp` and so on.

  -c --rdr-comm=< headless | thread | socket >
        The renderer commnunication method; use `headless` (default), `thread`, or `socket`.
            - `headless`: use the built-in headlesss renderer.
            - `thread`: use the built-in thread-based renderer.
            - `socket`: use the remote socket-based renderer;
              `purc` will connect to the renderer via Unix Socket or WebSocket.
  -u --rdr-uri=< renderer_uri >
        The renderer uri:
            - For the renderer comm method `headleass`,
              default value is `file:///dev/null`.
            - For the renderer comm method `thread`,
              default value is `edpt://localhost/cn.fmsoft.hvml.renderer/foil`.
            - For the renderer comm method `socket`,
              default value is `unix:///var/tmp/purcmc.sock`.

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

### 在多个行者中运行 HVML 应用程序

PurC 支持在多个行者中运行应用程序。在这里，一个 `runner` 是 `purc` 进程中的一个线程。

为此，您可以准备一个 JSON 文件或 eJSON 文件，该文件定义了应用程序、行者和初始 HVML 程序，以便在不同的行者中作为协程运行。

这里有一个例子：

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

假设您准备了所有 HVML 程序，并将上述 JSON 保存为 `cn.fmsoft.hvml.sample.json`，您可以通过以下方式运行 `purc`：

```bash
$ purc cn.fmsoft.hvml.sample.json
```

*请注意，当以这种方式运行应用程序时，您可以在解析 eJSON 文件时通过 `purc` 准备的变量 `$OPTS` 使用 eJSON 文件中的命令行选项。这给出了 PurC 引入的参数化 eJSON 的典型应用。*

例如，我们可以指定命令行选项：

```bash
$ purc --app=cn.fmsoft.hvml.sample my_app.ejson
```

然后使用 `my_app.ejson` 中 `--app` 指定的选项：

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

`my_app.ejson` 中出现的所有 `$OPTS.app` 都将替换为 `cn.fmsoft.hvml.sample`。

### 更多 HVML 示例

您可以在代码仓库 [HVML Documents](https://github.com/HVML/hvml-docs) 的目录 `samples/` 下找到更多 HVML 示例程序。

可以使用 `purc` 直接运行远程 HVML Documents 仓库中的示例：

```bash
$ purc --data-fetcher=remote https://github.com/HVML/hvml-docs/raw/master/samples/fibonacci/fibonacci-6.hvml
```

如果防火墙拒绝连接到 URL，请使用以下 URL：

```bash
$ purc --data-fetcher=remote https://gitlab.fmsoft.cn/hvml/hvml-docs/-/raw/master/samples/fibonacci/fibonacci-6.hvml
```

请注意，当 `purc` 尝试从远程 URL 加载 HVML 程序时，默认情况下，它将使用远程数据获取器。因此，您必须提前安装 PurC Fetcher。有关构建和安装 PurC Fetcher 的详细说明，请参阅 [PurC Fetcher](https://github.com/HVML/purc-fetcher)。

## Hacking PurC

### 当前状态

该项目于 2021 年 6 月创建。2021年。当前版本是 PurC 0.9.2。

*PurC 的主要目的是为您提供一个函数库来编写自己的 HVML 解释器。经过一年的开发，当前版本实现了 HVML 规范 V1.0 定义的几乎所有功能，还实现了由 HVML 预定义变量 V1.0 定义的几乎所有预定义动态变量。*

除了 HVML 解释器外，Purc 还为一般 C 程序提供了许多基本功能：

1. PurC 为变量管理提供了 API，这里的变量是 HVML 管理数据的方式。
2. PurC 提供了解析 JSON 和扩展 JSON 的 API。
3. PurC 提供了解析和评估参数化 eJSON 表达式的 API。
4. PurC 提供了用于解析 HTML 文档的 API。
5. PurC 提供了创建多个 HVML 行者的 API。
6. PurC 提供了用于解析 HVML 程序和调度运行它的 API。

您可以根据需要独立使用这些 API 组。

我们欢迎任何人参与开发和贡献!

### PurC 的源代码树

PurC 为 HVML 实现了解析器、解释器和一些内置的动态对象。它主要用 C/C++ 语言编写，将来还会提供 Python 和其他脚本语言的绑定。

PurC 的源代码树包含以下模块：

- `Source/PurC/include/`：全局头文件。
- `Source/PurC/include/private`：内部公共头文件。
- `Source/PurC/utils/`：一些基本和常见的实用程序。
- `Source/PurC/variant/`：变量的实现。
- `Source/PurC/vcm/`：变量创建模型树的操作。
- `Source/PurC/dvobjs/`：内置动态变量对象。
- `Source/PurC/ejson/`：eJSON 解析器的实现。eJSON 解析器读取 eJSON 并构建变量创建模型树。
- `Source/PurC/dom/`：DOM 树的实现。
- `Source/PurC/vdom/`：虚拟 DOM 树的实现。
- `Source/PurC/html/`：HTML 解析器的实现。HTML 解析器读取 HTML 文档或文档片段并构建 eDOM 树。
- `Source/PurC/hvml/`：HVML 解析器的实现。HTML 解析器读取 HVML 文档并构建 vDOM 树。
- `Source/PurC/xgml/`：XGML 解析器的实现（到目前为止尚未实现）。XGML 解析器读取 XGML 文档或文档碎片并构建 eDOM 树。
- `Source/PurC/xml/`：XML 解析器（到目前为止尚未实现）。XML 解析器解析 XML 文档或文档片段，并构建 eDOM 树。
- `Source/PurC/instance/`：PurC 实例和会话的操作。
- `Source/PurC/fetchers/`：从各种数据源（文件、HTTP、FTP 等）获取数据的数据获取器。
- `Source/PurC/executors/`：内部执行程序的实现。
- `Source/PurC/interpreter/`：vDOM 解释器。
- `Source/PurC/pcrdr/`：连接到渲染器的管理。
- `Source/PurC/ports/`：不同操作系统（如 POSIX 兼容系统或 Windows）的移植。
- `Source/PurC/bindings/`：Python、Lua 和其他编程语言的绑定。
- `Source/ExtDVObjs/math/`：外部动态变量对象 `$MATH` 的实现。
- `Source/ExtDVObjs/fs/`：外部动态变量对象 `$FS` 和 `$FILE` 的实现。
- `Source/CSSEng/`：CSS 解析和选择引擎，源自 NetSurf 项目的 libcss。
- `Source/DOMRuler/`：使用 CSSEng 布局和风格化 DOM 树的函数库。
- `Source/RemoteFetcher/`：PurC Remote Fetcher 使用的函数库。
- `Source/WTF/`：来自 WebKit 的简化 WTF（Web 模板框架）。
- `Source/cmake/`：cmake 模块。
- `Source/ThirdParty/`：第三方库，如 `gtest`。
- `Source/test/`：单元测试程序。
- `Source/Samples/api`：使用 PurC API 的示例。
- `Source/Samples/hvml`：HVML 示例程序。
- `Source/Executables/`：可执行文件。
- `Source/Executables/purc`：基于 PurC 的独立 HVML 解释器/调试器，这是一个交互式命令行程序。
- `Source/Executables/purc-fetcher`：PurC Remote Fetcher 的最终可执行文件。
- `Source/Tools/`：维护此项目的工具或脚本。
- `Source/Tools/aur`：AUR 包打包脚本。
- `Source/Tools/debian`：DEB 包打包脚本。
- `Documents/`：一些面向开发人员的文档。

请注意，PurC 的 HTML 解析器和 DOM 操作来自：

- [Lexbor](https://github.com/lexbor/lexbor)，它是根据 Apache 2.0 许可证授权的。

### 待办事项列表

1. HVML 1.0 特性尚未实现。
2. HVML 1.0 预定义变量尚未实现。
3. 更多测试或测试用例。
4. 更多示例。
5. 将 PurC 移植到 Windows。

有关详细的待办事项列表，请参阅[待办事项列表](https://github.com/HVML/PurC/blob/master/TODO.md)。

### 其他文件

有关发布说明，请参阅[发布说明](https://github.com/HVML/PurC/blob/master/RELEASE-NOTES.md)。

有关社区行为，请参阅[行为规范](https://github.com/HVML/PurC/blob/master/Documents/CODE_OF_CONDUCT.md)。

有关编码约定，请参阅[编码约定](https://github.com/HVML/PurC/blob/master/Documents/CODING_CONVENTION.md)。

## 作者和贡献者

- 魏永明：架构师。
- 薛淑明：主要开发者，大多数模块和 Purc Fetcher 的维护者。
- 徐晓宏：主要开发者，实现了变量的大部分功能和 HVML 解释器的大部分功能。
- 刘新：开发者，外部动态变量对象 `FILE` 的维护者。
- 耿越：提交者，实现了一些内置动态变量对象。

## 版权信息

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

### DOMRuler

Copyright (C) 2021, 2022 FMSoft <https://www.fmsoft.cn>

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

### purc-fetcher

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

## 商标

1) `HVML` 是[飞漫软件]在中国和其他国家或地区的注册商标。

![HVML](https://www.fmsoft.cn/application/files/8116/1931/8777/HVML256132.jpg)

2) `呼噜猫` 是[飞漫软件]在中国和其他国家或地区的注册商标。

![呼噜猫](https://www.fmsoft.cn/application/files/8416/1931/8781/256132.jpg)

3) `Purring Cat` 是[飞漫软件]在中国和其他国家或地区的注册商标。

![Purring Cat](https://www.fmsoft.cn/application/files/2816/1931/9258/PurringCat256132.jpg)

4) `PurC` 是[飞漫软件]在中国和其他国家或地区的注册商标。

![PurC](https://www.fmsoft.cn/application/files/5716/2813/0470/PurC256132.jpg)

5) `xGUI` 是[飞漫软件]在中国和其他国家或地区的注册商标。

![xGUI](https://www.fmsoft.cn/application/files/cache/thumbnails/7fbcb150d7d0747e702fd2d63f20017e.jpg)

