HybridOS Specification 0005  
Topic: HybridOS Code and Development Convention  
Author: Vincent Wei  
Category: General
Date: November 2018  
Status: Proposal

*Copyright Notice*

Copyright (C) 2018 [FMSoft Technologies]  
All Rights Reserved.

## Introduction

HybridOS integrates many third-party open source libraries in different
languages. So it is hard to make a unified coding style for all components
of HybridOS.

Anyway, we have the following must rules:

* Comply with the existed code convention of the software when you are
  modifying the software.
* For any new components of HybridOS, use the coding style described in this
  document.

## C

For C program, our coding style mainly complies with
[Linux Kernel Coding Style], but with the following exceptions:

* Always use spaces for indentation instead of tabs, and intent 4 spaces
  at one time.
* Always use Unix convention (all lowercase, with underscores between words)
  for static functions, use upper camel casing for application interface
  functions, and use lower camel casing for internal interfaces.
* Use Unix convention for local variables; use lower camel casing for 
  parameters of application interface functions; use lower camel casing with
  a prefix `_` for static global variables; use Unix convention with a 
  prefix `__` for extern global variables.
* Avoid excessive use of typedef, use basic C/C++ types always when it is
  possible. Only use typedef in the following cases defined by
  [Linux Kernel Coding Style]:
    1. Totally opaque objects (where the typedef is actively used to hide
       what the object is).
    1. Clear integer types, where the abstraction helps avoid confusion
       whether it is int or long.
    1. When you use sparse to literally create a new type for type-checking.

The following code is an example:

    static BOOL get_json_value_int (const cJSON* json, const char* field,
        int* value)
    {
        cJSON* item = cJSON_GetObjectItem (json, field);
        if (item == NULL || item->type != cJSON_Number) {
            return false;
        }

        *value = item->valueint;
        return true;
    }

    extern struct language_zipped_strings __ngux_map_lang_to_zipped_bytes [];

    static char* _resPkgStringsBucket = NULL;

    BOOL SetCurrentLang (int langId)
    {
        if (langId < 0 || langId >= MAX_LANGID)
            return FALSE;

        if (__ngux_map_lang_to_zipped_bytes [langId].zipped_bytes == NULL) {
            _ERR_PRINTF ("ResPkgManager::setCurrentLang: no strings for language: %d.\n", langId);
            return FALSE;
        }

        if (_resPkgStringsBucket == NULL) {
            size_t max_bucket_size = 0;
            for (int i = 0; i < MAX_LANGID; i++) {
                size_t current_size = __ngux_map_lang_to_zipped_bytes [i].origin_size;
                if (current_size > max_bucket_size) {
                    max_bucket_size = current_size;
                }
            }

            if (max_bucket_size == 0) {
                _ERR_PRINTF ("ResPkgManager::setCurrentLang: no any language defined.\n");
                return FALSE;
            }

            _resPkgStringsBucket = (char*)malloc (max_bucket_size);
            if (_resPkgStringsBucket == NULL) {
                return FALSE;
            }
        }

        ...

        return TRUE;
    }

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

The following code is an example:

    ResPackage* ResPkgManager::getPackage (int id)
    {
        ResPkgList::iterator it;
        ResPackage* package = NULL;

        for (it = m_packageList.begin(); it != m_packageList.end(); it++) {
            package = (*it);
            if (package && (package->getId() == id)) {
                return package;
            }
        }

        return NULL;
    }

## Python

## HVML

## JavaScript

## Editor Settings

### Vim

Use the following commands in your `~/.vimrc` file:

    set expandtab
    set tabstop=4
    set shiftwidth=4
    sytax on

## Development Environment

Host operating system: Ubuntu 18.04 LTS

Please make sure that you have installed the following packages:

1. Building tools:
    * git
    * gcc/g++
    * binutils
    * autoconf/automake
    * libtool
    * make
    * cmake
    * pkg-config
2. Dependent libraries:
    * libgtk2.0-dev
    * libjpeg-dev
    * libpng-dev
    * libfreetype6-dev
    * libsqlite3-dev
    * libxml2-dev

[Beijing FMSoft Technologies Co., Ltd.]: https://www.fmsoft.cn
[FMSoft Technologies]: https://www.fmsoft.cn
[HybridOS Official Site]: https://hybridos.fmsoft.cn

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
