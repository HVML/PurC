# Release Notes

- [Version 0.8.0](#version-080)

## Version 0.8.0

On July 31, 2022, HVML Community announces the availability of PurC 0.8.0,
   which is the first beta release of PurC 1.0.x.

This version implements almost all features defined by [HVML Specifiction V1.0],
      and also implements almost all predefined dynamic variables defined by [HVML Predefined Variables V1.0].

For any bugs, incompatibilities, and issues, please report to <https://github.com/HVML/PurC/issues>.

### Known Bugs

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

