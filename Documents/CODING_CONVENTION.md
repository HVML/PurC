# Coding Convention

Author: Vincent Wei  
Date: July 2022

**Copyright Notice**

Copyright (C) 2022 HVML Community

**Table of Contents**

[//]:# (START OF TOC)

- [Introduction](#introduction)
- [C](#c)
- [C++](#c)
- [Python](#python)
- [HVML](#hvml)
- [Editor Settings](#editor-settings)
   + [Vim](#vim)
- [Development Environment](#development-environment)

[//]:# (END OF TOC)

## Introduction

PurC is written mainly in C language, with a small amount of C++ code and
some Python scripts to generate C code automatically.

In this document, we describe the coding convention when we writing code
for PurC.

## C

As a library, PurC always provides interfaces in C language. PurC needs
at least a C99-compliant compiler and C standard library, and uses some
features of C11:

- The header `stdatomic.h`.

For C program, our coding style mainly complies with
[Linux Kernel Coding Style], but with the following exceptions:

1) Always use spaces for indentation instead of tabs, and intent 4 spaces
  at one time.

2) Always use Unix convention (all lowercase, with underscores between words)
  for functions, varaibles.

3) For any function or type name as an API, we use `purc_` or `pc<modules>_`
  (like `pcutils_`) as the prefix.

4) For any external function for internal use, delcare it with `WTF_INTERNAL`.

5) Avoid excessive use of typedef, use basic C/C++ types always when it is
  possible. Only use typedef in the following cases:
    1. Totally opaque objects (where the typedef is actively used to hide
       what the object is).
       We use `_t` as the postfix for the type name only for the pointer to
       a structure for this purpose.
    1. Clear integer types, where the abstraction helps avoid confusion
       whether it is int or long. We use `_k` as the postfix for the type name
       of an enum in this sitiuation.
    1. When you use sparse to literally create a new type for type-checking.

6) Try your best to avoid static/global variables. If you have to use,
  use `_` as the prefix of an static/global variable, and use `__` as
  the prefix of an external global variable.

Please see the source file [move-heap.c](/Source/PurC/variant/move-heap.c),
which shows the typical coding style.

### Coding Conventions not Recommended

1) Avoid to use VLA for possible large array

2) Avoid to use a large local array in a function

3) Avoid to use `do { ... } while (0);` in source file

## C++

For C++ program, our coding style mainly complies with [Google C++ Style Guide],
but with the following exceptions:

* Currently, code should target C++17, i.e., should not use C++20 features.
* Only use libraries and language extensions from C++11 when appropriate.
* Always use spaces instead of tabs for indentation, and the width of one
  intentation should be 4-space long.
* For structure data menubers, always use Unix convention without prefix.
* For protected or private class data members, use Unix convention with
  the prefix of `m_`.
* For public class data members, use lower camel casing with the prefix of `m_`.
* For class functions, use lower camel casing.
* Use Unix convention for local variables; use lower camel casing for
  parameters of application interface functions; use lower camel casing with
  a prefix `_` for static global variables; use Unix convention with a
  prefix `__` for extern global variables.
* Avoid excessive use of typedef, use basic C/C++ types always when
  it is possible. Only use typedef in the following cases defined by
[Linux Kernel Coding Style]:
    1. Totally opaque objects (where the typedef is actively used to hide
       what the object is).
    1. Clear integer types, where the abstraction helps avoid confusion
       whether it is int or long.
    1. When you use sparse to literally create a new type for type-checking.

The following code gives an example:

```cpp
```

## Python

## HVML

## Editor Settings

### Vim

Use the following commands in your `~/.vimrc` file:

    set expandtab
    set tabstop=4
    set shiftwidth=4
    sytax on

## Development Environment

Host operating system: Ubuntu 20.04 LTS or later.

Please make sure that you have installed the following packages:

1. Building tools:
    * git
    * gcc/g++
    * binutils
    * cmake
    * pkg-config

[Beijing FMSoft Technologies Co., Ltd.]: https://www.fmsoft.cn
[FMSoft Technologies]: https://www.fmsoft.cn
[HVML Official Site]: https://hvml.fmsoft.cn

[MiniGUI]: http:/www.minigui.com
[WebKit]: https://webkit.org
[HTML 5.3]: https://www.w3.org/TR/html53/
[DOM Specification]: https://dom.spec.whatwg.org/
[WebIDL Specification]: https://heycam.github.io/webidl/
[CSS 2.2]: https://www.w3.org/TR/CSS22/
[CSS Box Model Module Level 3]: https://www.w3.org/TR/css-box-3/
[Linux Kernel Coding Style]: https://www.kernel.org/doc/html/latest/process/coding-style.html
[Google C++ Style Guide]: https://google.github.io/styleguide/cppguide.html

[HybridOS Architecture]: HybridOS-Architecture
[HybridOS Code and Development Convention]: HybridOS-Code-and-Development-Convention
