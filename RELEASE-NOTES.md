# Release Notes

- [Version 0.8.2](#version-082)
- [Version 0.8.0](#version-080)

## Version 0.8.2

On Sep. 04, 2022, HVML Community announces the availability of PurC 0.8.2,
   which is also the second alpha release of PurC 1.0.x.

For any bugs, incompatibilities, and issues, please report to <https://github.com/HVML/PurC/issues>.

### What's new in version 0.8.2

In this version, we fixed some bugs and made som enhancements:

* BUGFIXING:
   - The content of an `iterate` element may be evaluated twice.
   - Incorrect evaluation logic of a CJSONEE with `&&` and `||`.
   - Refactor eJSON parser to support the varoius string patterns.
   - Fix bug of percent escaped path for `file://` URL.
* ENHANCEMENTS:
   - Provide support for channel, which can act as an inter-coroutine communication (ICC) mechanism. See Section 3.2.7 of [HVML Predefined Variables V1.0].
   - Support for `rdrState:connLost` event on `$CRTN`.
   - Implement new APIs: `purc_coroutine_dump_stack`.
   - Support for use of an element's identifier as the value of the `at` attribute in an `init` element.
   - Improve the element `init` to make the attribute `as` be optional, so we can use `init` to initilize a data but do not bind the data to a variable.
   - Implement the `request` tag (only inter-runner request).
   - Provide support for `type` attribute of the element `archetype`. It can be used to specify the type of the template contents, for example, `plain`, `html`, `xgml`, `svg`, or `mathml`.
   - Support for prefix for foreign tag name. See Section 3.1.1 of [HVML Specifiction V1.0].
   - Support for using Unihan characters in variable names and property/method names. See Section 2.2.2 of [HVML Specifiction V1.0].
* OPTIMIZATIONS:
   - Optimize the content evaluation of foreign elements: make sure there is only one text node after evaluating the contents `$< Hello, world! --from COROUTINE-$CRTN.cid`.
* IMPROVEMENT:
   - When the contents of the target document is very large, send the contents by using operations `writeBegin`, `writeMore`, and `writeEnd`.
   - Raise exceptions if encounter errors when the fetcher failed to load a resource of a given URL.
* TUNING:

## Version 0.8.0

On July 31, 2022, HVML Community announces the availability of PurC 0.8.0,
   which is also the first alpha release of PurC 1.0.x.

This version implements almost all features defined by [HVML Specifiction V1.0],
      and also implements almost all predefined dynamic variables defined by [HVML Predefined Variables V1.0].

For any bugs, incompatibilities, and issues, please report to <https://github.com/HVML/PurC/issues>.

### Known Bugs in Version 0.8.0

1. The content of an `iterate` element may be evaluated twice.
1. Some EJSON expressions within `""` are evaluated incorrectly.
1. Incorrect evaluation logic of a CJSONEE with `&&` and `||`.


[HVML Community]: https://hvml.fmsoft.cn

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

[HVML Specifiction V1.0]: https://github.com/HVML/hvml-docs/blob/master/zh/hvml-spec-v1.0-zh.md
[HVML Predefined Variables V1.0]: https://github.com/HVML/hvml-docs/blob/master/zh/hvml-spec-predefined-variables-v1.0-zh.md

