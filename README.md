# PurC

`PurC` is the Prime hVml inteRpreter for C language. It is also
the abbreviation of `Purring Cat`, while Purring Cat is the nickname
and the mascot of HVML.

**Table of Contents**

[//]:# (START OF TOC)

- [Introduction](#introduction)
- [What's HVML](#whats-hvml)
   + [Problems](#problems)
   + [Our Solution](#our-solution)
   + [Application Framework](#application-framework)
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

`PurC` is the Prime hVml inteRpreter for C language. It is also
the abbreviation of `Purring Cat`, while Purring Cat is the nickname
and the mascot of HVML, which is a new-style programming language proposed
by [Vincent Wei].

Although we designed HVML originally as a programming language to rapidly
develop GUI applications based on Web front-end technology in the C/C++
runtime environment, the developer can also use HVML as a general script
language in other scenarios.

PurC implements all features defined by [HVML Specifiction V1.0] in C language.
PurC also implements all predefined dynamic variables defined by
[HVML Predefined Variables V1.0].

PurC provides support for Linux and macOS. The support for Windows is
on the way. We welcome others to port HVML to other platforms.

You can use PurC to run a HVML program by using the command line tool `purc`, or
use PurC as a library to build your own HVML interpreter. We release PurC under
LGPLv3, so it is free for commercial use if you follow the terms of LGPLv3.

For documents or open source tools of HVML, please refer to the following repositories:

- HVML Documents: <https://github.com/HVML/hvml-docs>.
- PurC (the Prime hVml inteRpreter for C language): <https://github.com/HVML/purc>.
- PurC Fetcher (the remote data fetcher for PurC): <https://github.com/HVML/purc-fetcher>.
- PurCMC (an HVML renderer in text-mode): <https://github.com/HVML/purc-midnight-commander>.
- xGUI Pro (an advanced HVML renderer based on WebKit): <https://github.com/HVML/xgui-pro>.

## What's HVML

The original design goal of HVML is to allow developers who are familiar with
C/C++, Python, or other programming languages to easily develop GUI applications
by using web front-end technologies (such as HTML/SVG, DOM and CSS), instead of
using JavaScript programming language in the browser or Node.js.

We achieved this design goal and also designed HVML as a new-style and
general-purpose programming language. Now, we can not only use HVML as
a programming language to rapidly develop GUI applications based on Web
front-end technology in the C/C++ runtime environment, but also use HVML
as a general script language.

### Problems

With the development of Internet technology and applications, the Web front-end
development technology around HTML/CSS/JavaScript has evolved rapidly.
Since 2019, frameworks based on virtual DOM (Document Object Model) technology
have been favored by front-end developers, such as [React.js] and [Vue.js].

The so-called "virtual DOM" refers to a front-end application that uses
JavaScript to create and maintain a virtual DOM tree, and the application
scripts do not directly manipulate the real DOM tree.

The virtual DOM technology provides the following benefits:

1. The script does not directly manipulate the real DOM tree. On the one hand,
   the existing framework simplifies the complexity of front-end development,
   on the other hand, it reduces the frequent operations on the DOM tree through
   dynamic modification of page content by optimizing the operation of the real DOM tree,
   thus improving page rendering efficiency and user experience.
2. With the virtual DOM technology, the modification of a certain data
   by the program can directly be reflected on the content of the data-bound page,
   and the developer does not need to actively or directly call the relevant
   interface to operate the DOM tree. This technology provides so-called
   "responsive" programming, which greatly reduces the workload of developers.

On the other side, front-end frameworks represented by React.js and Vue.js have
achieved great success, but have the following deficiencies and shortcomings:

1. These technologies are based on mature Web standards and require browsers
   that fully support the relevant front-end specifications to run, so they
   cannot be applied to other occasions. For example, if you want to use
   this kind of technology in Python scripts, there is currently no direct
   solution; another example is in traditional GUI application programming,
   the developers cannot benefit from this technology.
2. These technologies implement data-based condition and loop flow controls
   by introducing virtual attributes such as `v-if`, `v-else`, and `v-for`.
   However, this method brings a sharp drop in code readability, which in turn
   brings drop of code maintainability. Below is an example:

```html
<div v-if="Math.random() > 0.5">
  Now you see "{{ name }}"
</div>
<div v-else>
  Now you don't
</div>
```

### Our Solution

During the development of [HybridOS], [Vincent Wei] proposed a new-style,
general-purpose, and easy-to-learn programming language called `HVML`.

HVML is a programmable markup language. Like HTML, HVML uses markups to define
program structure and data, but unlike HTML, HVML is programmable and dynamic.

HVML realizes the dynamic generation and update function of data and XML/HTML
documents through a limited number of action tags and dynamic JSON expressions
that can be used to define attributes and content; HVML also provides mechanisms
to interact with the runtime of an existing programming language, such as
C/C++, Python, Lua, etc., so as to provide strong technical support for these
programming languages to utilize Web front-end technology outside the browser.
From this perspective, HVML can also be regarded as a glue language.

The classical `helloworld` program in HVML looks like:

```html
<!DOCTYPE hvml>
<hvml target="void">

    $STREAM.stdout.writelines('Hello, world!')

</hvml>
```

The HVML program above will print the following line on your terminal:

```
Hello, world!
```

Obviously, the key statement of the above program is

```js
$STREAM.stdout.writelines('Hello, world!')
```

This statement called the `writelines` method of `$STREAM.stdout`, and the
method printed the `Hello, world!` to STDOUT, i.e., your terminal.

Now we rewrite the above program a little more complicated to have the following
features:

- Output a valid HTML document or a simple text line according to
  a startup option.
- Support localization according to the current system locale.

Please read the code below and the comments carefully:

```html
<!DOCTYPE hvml>

<!-- $REQUEST contains the startup options -->
<hvml target="$REQUEST.target">
  <body>

    <!--
        $SYSTEM.locale returns the current system locale like `zh_CN'.
        This statement load a JSON file which defined the map of
        localization messages, like:
        {
            "Hello, world!": "世界，您好！"
        }
    -->
    <update on="$T.map" from="messages/$SYSTEM.locale" to="merge" />

    <!--
        This statement defines an operation set, which output
        an HTML fragment.
    -->
    <define as="output_html">
        <h1>HVML</h1>
        <p>$?</p>
    </define>

    <!--
        This statement defines an operation set, which output
        a text line to STDOUT.
    -->
    <define as="output_void">
        <choose on=$STREAM.stdout.writelines($?) />
    </define>

    <!--
        This statement includes one of the operation sets defined above
        according to the value of `target` attribute of `hvml` element,
        and pass the result returned by `$T.get('Hello, world!')`.
    -->
    <include with=${output_$HVML.target} on=$T.get('Hello, world!') />

  </body>
</hvml>
```

The HVML program above will generate a HTML document if the current system
locale is `zh_CN` and the value of the startup option `target` is `html`:

```html
<html>
  <body>
        <h1>HVML</h1>
        <p>世界，您好！</p>
  </body>
</html>
```

But if the value of the startup option `target` is `void`, the HVML program
above will print the following line on your terminal:

```
世界，您好！
```

With the simple samples above, you can see what's interesting about HVML.

In essence, HVML provides a new way of thinking to solve the previous problem:

- First, it introduces web front-end technologies (HTML, XML, DOM, CSS, etc.)
  into other programming languages, rather than replacing other programming
  languages with JavaScript.
- Second, it uses an HTML-like markup language to manipulate elements,
  attributes, and styles in Web pages, rather than JavaScript.
- In addition, in the design of HVML, we intentionally use the concept of
  data-driven, so that HVML can be easily combined with other programming
  languages and various network connection protocols, such as data bus,
  message protocol, etc. In this way, developers use a programming language
  with which is familiar by them to develop the non-GUI part of the application,
  and all the functions of manipulating the GUI are handed over to HVML, and
  the modules are driven by the data flowing between them. While HVML provides
  the abstract processing capability of the data flow.

Although HVML was originally designed to improve the efficiency of GUI
application development, it can actually be used in more general scenarios -
HVML can be used as long as the output of the program can be abstracted into
one or more tree structures; even we can use HVML like a common script
language.

Essentially, HVML is a new-style programming language with a higher level of
abstraction than common script languages such as JavaScript or Python.
Its main features are:

- Simple design. HVML defines the complete set of instructions for operating
  an abstract stack-based virtual machine using only a dozen tags.
  Each line of code has clear semantics through verb tags, preposition
  attributes, and adverb attributes that conform to English expression habits.
  This will help developers write program code with excellent readability.
- Data driven. On the one hand, HVML provides methods for implementing
  functions by manipulating data. For example, we can use the update action to
  manipulate a field in the timer array to turn a timer on or off without
  calling the corresponding interface. On the other hand, the HVML language is
  committed to connecting different modules in the system through a unified data
  expression, rather than realizing the interoperation between modules through
  complex interface calls. These two methods can effectively avoid the interface
  explosion problem existing in traditional programming languages. To achieve
  the above goals, HVML provides extended data types and flexible expression
  processing capabilities on top of JSON, a widely used abstract data
  representation.
- Inherent event-driven mechanism. Unlike other programming languages, the HVML
  language provides language-level mechanisms for observing data, events, and
  even observing changes in the result of an expression. On this basis,
  developers can easily implement concurrency or asynchronous programming that
  is difficult to manage in other programming languages without caring about
  the underlying implementation details.
- New application framework. Through HVML's unique application framework, we
  delegate performance-critical data processing to an external program or server,
  and the interaction with the user is handled by an independent renderer, and
  the HVML program is responsible for gluing these different system components.
  On the one hand, HVML solves the problem of difficult and efficient
  interoperability between system components developed in different programming
  languages, so that the advantages of each component can be fully utilized and
  the value of existing software assets can be protected; on the other hand,
  once the application framework provided by HVML is adopted, we can minimize
  the coupling problem between different components.

In short, HVML provides a programming model that is different from traditional
programming languages. On the basis of data-driven, HVML provides a more
systematic and complete low-code (which means using less code to write programs)
programming method.

### Application Framework

When we used HVML to build the framework for GUI applications,
we got a totally different framework other than Java, C#, or Swift.

In a complete HVML-based application framework, a standalone UI renderer
is usually included. Developers write HVML programs to manipulate the page
content that describes the user interface, and the page content is finally
processed by the renderer and displayed on the screen. The HVML program runs
in the HVML interpreter, which can interact easily with the runtime environment
of other existing programming languages. The HVML program receives data or
events generated by other foreign programs or the renderer, and converts it
into the description of the UI or changes of UI according to the instructions
of the HVML program.

With this design, we separate all applications involving the GUI into two
loose modules:

- First, a data processing module independent of the user interface, developers
can use any programming language and development tools they are familiar with
to develop this module. For example, when it comes to artificial intelligence
processing, developers choose C++ or Python; in C++ code, apart from loading
HVML programs, developers do not need to consider anything related to interface
rendering and interaction, such as creating a button or clicking a menu item.
Developers only need to prepare the data needed to render the user interface
in the C++ code, and these data are usually represented by JSON.

- Second, one or more programs written in the HVML language (HVML programs) to
complete the manipulation of the user interface. The HVML program generates
the description information of the user interface according to the data provided
by the data processing module, and updates the user interface according to the
user's interaction or the calculation results obtained from the data processing
module, or drives the data processing module to complete certain tasks according
to the user's interaction.

In this way, the HVML application framework liberates the code for manipulating
interface elements from the tranditional design pattern of calling interfaces
such as C/C++, Java, C#  and uses HVML code instead. HVML uses a tag language
similar to HTML to manipulate interface elements. By hiding a lot of details,
it reduces the program complexity caused by directly using low-level
programming languages to manipulate interface elements.

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

