# Release Notes

- [Version 0.9.6](#version-096)
- [Version 0.9.5](#version-095)
- [Version 0.9.4](#version-094)
- [Version 0.9.2](#version-092)
- [Version 0.9.0](#version-090)
- [Version 0.8.2](#version-082)
- [Version 0.8.0](#version-080)

## Version 0.9.6

On Feb. 25, 2023, HVML Community announces the availability of PurC 0.9.6,
   which is also the sixth alpha release of PurC 1.0.x.

For bugs, incompatibilities, and issues, please report to <https://github.com/HVML/PurC/issues>.

### What's new in version 0.9.6

In this version, we fixed some bugs and made som enhancements:

* CHANGES:
   - (N/A).
* ADJUSTMENTS:
* ENHANCEMENTS:
   - Complete the implementation of the predefined variables `$DOC`.
   - Improve `init` to load content with MIME type `text/html` as a document entity.
   - Improve support for the attribute `in`, so we can use a value like `> p` to specify an descendant as the current document position.
   - Enhance Foil renderer to support `meter` and `progress` elements.
* OPTIMIZATIONS:
   - Optimize the variant moudule.
   - Optimize the evaluation of a VCM tree to descrease uses of `malloc()` and `free()`.
* BUGFIXES:
   - Fix some minor bugs in Foil when laying the redering boxes.
* SAMPLES:
   - New sample `spider-headline.hvml`: fetching the latest headlines from news websites.
   - New sample `foil-progress.hvml`: Show usage of `progress` tag in Foil renderer.
   - New sample `foil-meter.hvml`: Show usage of `meter` tag in Foil renderer.

## Version 0.9.5

On Jan. 10, 2023, HVML Community announces the availability of PurC 0.9.5,
   which is also the fifth alpha release of PurC 1.0.x.

For bugs, incompatibilities, and issues, please report to <https://github.com/HVML/PurC/issues>.

### What's new in version 0.9.5

In this version, we fixed some bugs and made som enhancements:

* CHANGES:
   - (N/A).
* ADJUSTMENTS:
   - Tune to skip some test cases if there are no commands needed installed.
* ENHANCEMENTS:
   - Add a new HVML sample: `Source/Sample/hvml/file-manager.hvml` to browse files in local system.
   - Improve the built-in Foil renderer to support basic layouts and CSS properties about text style.
* OPTIMIZATIONS:
   - Optimize the socket communication with a remote HVML renderer.
* BUGFIXES:
   - (N/A).

## Version 0.9.4

On Dec. 30, 2022, HVML Community announces the availability of PurC 0.9.4,
   which is also the fifth alpha release of PurC 1.0.x.

For bugs, incompatibilities, and issues, please report to <https://github.com/HVML/PurC/issues>.

### What's new in version 0.9.4

In this version, we fixed some bugs and made som enhancements:

* CHANGES:
   - Rename some APIs.
   - Tune almost all API description.
* ADJUSTMENTS:
   - (N/A).
* ENHANCEMENTS:
   - Improve the implementation of the element `update`.
   - Full support for `request` element.
   - Add new test cases or enhance test cases.
   - Enhance Foil to support more properties and layouts except for table.
* OPTIMIZATIONS:
   - (N/A).
* IMPROVEMENTS:
   - Tune some API descriptions.
* BUGFIXES:
   - The bug reported in Issue #42.

## Version 0.9.2

On Nov. 30, 2022, HVML Community announces the availability of PurC 0.9.2,
   which is also the forth alpha release of PurC 1.0.x.

For bugs, incompatibilities, and issues, please report to <https://github.com/HVML/PurC/issues>.

### What's new in version 0.9.2

In this version, we fixed some bugs and made som enhancements:

* CHANGES:
   - Rename some APIs.
* ADJUSTMENTS:
   - Tune the serialization format of eDOM.
* ENHANCEMENTS:
   - Support for triple-single-quote syntax.
   - The HEEs which are embedded in a string enclosed by two triple-double-quotes will be evaluated.
   - Support line comments in CHEE.
   - Support for the tuple.
   - Support for using string constants to define exception in `catch` and `except` tags.
   - In the implementation of predefined variables, use the interfaces for linear container instead of array.
   - Add a new API: `purc_variant_make_atom()`
   - Support for using a URL query string (in RFC 3986) for the request data of `purc`.
   - Foil now supports more CSS properties: `white-space`, `list-style-type`, and so on.
* OPTIMIZATIONS:
* IMPROVEMENTS:
   - Tune some API descriptions.
* BUGFIXES:
   - eJSON parser now raises errors for C0 control characters.
   - Some expressions in `hvml` and `iterate` were evaluated twice incorrectly.

## Version 0.9.0

On Otc. 31, 2022, HVML Community announces the availability of PurC 0.9.0,
   which is also the third alpha release of PurC 1.0.x.

For bugs, incompatibilities, and issues, please report to <https://github.com/HVML/PurC/issues>.

### What's new in version 0.9.0

In this version, we fixed some bugs and made som enhancements:

* CHANGES:
   * Rename type `purc_rdrprot_t` to `purc_rdrcomm_t`.
   * Rename macro `PURC_RDRPROT_HEADLESS` to `PURC_RDRCOMM_HEADLESS`.
   * Rename macro `PURC_RDRPROT_THREAD` to `PURC_RDRCOMM_THREAD`.
   * Rename macro `PURC_RDRPROT_PURCMC` to `PURC_RDRCOMM_SOCKET`.
   * Rename macro `PURC_RDRPROT_HIBUS` to `PURC_RDRCOMM_HIBUS`.
   * Rename the field `renderer_prot` of `struct purc_instance_extra_info` to `renderer_comm`.
   * Change the value of macro `PCRDR_PURCMC_PROTOCOL_VERSION` to 110.
   * Rename function `pcrdr_conn_protocol()` to `pcrdr_conn_comm_method()`.
   * Rename function `pcrdr_purcmc_connect()` to `pcrdr_socket_connect()`.
   * Rename function `pcrdr_purcmc_read_packet()` to `pcrdr_socket_read_packet()`.
   * Rename function `pcrdr_purcmc_read_packet_alloc()` to `pcrdr_socket_read_packet_alloc()`.
   * Rename function `pcrdr_purcmc_send_packet()` to `pcrdr_socket_send_packet()`.
   * Rename macro `PCA_ENABLE_RENDERER_PURCMC` to `PCA_ENABLE_RENDERER_SOCKET`.
* ADJUSTMENTS:
   * Use `edpt://` instead of `@` as the schema of an endpoint URI.
   * Merge repo of `DOM Ruler` to `PurC` (under `Source/CSSEng` and `Source/DOMRuler`).
   * Merge repo of `PurC Fetcher` to `PurC` (under `Source/RemoteFetcher`).
* ENHANCEMENTS:
   * Basic support for the new variant type: tuple.
   * Improve the implementation of the element `bind`:
      - The support for the adverb attribute `constantly`.
      - The support for the substituting expression.
   * Improve the element `init` to make the attribute `at` support `_runner`, so we can create a runner-level variable.
   * Improve the data fetcher to generate the progress events when fetching data.
   * Improve the function to get data from remote data fetcher:
      - The element `archetype`: support for `src`, `param`, and `method` attributes.
      - The element `archedata`: support for `src`, `param`, and `method` attributes.
      - The element `execpt`: support for `src`, `param`, and `method` attributes.
      - The element `init`: support for `from`, `with`, and `via` attrigbutes.
      - The element `define`: support for `from`, `with`, and `via` attributes.
      - The element `update`: support for `from`, `with`, and `via` attributes.
   * Support for the equivalence of the context variable `<`: `~`.
   * Support for the equivalences and/or abbreviations of some adverb attributes.
   * Support for the new preposition attribute: `idd-by`.
   * A simple built-in HTML renderer (Foil) for text terminal via `THREAD` channel.
* OPTIMIZATIONS:
* IMPROVEMENTS:
* BUGFIXES:

## Version 0.8.2

On Sep. 29, 2022, HVML Community announces the availability of PurC 0.8.2,
   which is also the second alpha release of PurC 1.0.x.

For bugs, incompatibilities, and issues, please report to <https://github.com/HVML/PurC/issues>.

### What's new in version 0.8.2

In this version, we fixed some bugs and made som enhancements:

* BUGFIXES:
   - The content of an `iterate` element may be evaluated twice.
   - Incorrect evaluation logic of a CJSONEE with `&&` and `||`.
   - Refactor eJSON parser to support the varoius string patterns.
   - Fix a bug about a percent escaped path for `file://` URL.
* ENHANCEMENTS:
   - Provide support for channel, which can act as an inter-coroutine communication (ICC) mechanism. See Section 3.2.7 of [HVML Predefined Variables V1.0].
   - Support for `rdrState:connLost` event on `$CRTN`.
   - Implement new APIs: `purc_coroutine_dump_stack`.
   - Support for use of an element's identifier as the value of the `at` attribute in an `init` element.
   - Improve the element `init` to make the attribute `as` be optional, so we can use `init` to initilize a data but do not bind the data to a variable.
   - Implement the `request` tag (only inter-runner request).
   - Provide support for `type` attribute of the element `archetype`. It can be used to specify the type of the template contents, for example, `plain`, `html`, `xgml`, `svg`, or `mathml`.
   - Support for the prefix for a foreign tag name. See Section 3.1.1 of [HVML Specifiction V1.0].
   - Support for using Unihan characters in variable names and property/method names. See Section 2.2.2 of [HVML Specifiction V1.0].
* OPTIMIZATIONS:
   - Optimize the content evaluation of foreign elements: make sure there is only one text node after evaluating the contents `$< Hello, world! --from COROUTINE-$CRTN.cid`.
* IMPROVEMENTS:
   - When the contents of the target document is very large, send the contents by using operations `writeBegin`, `writeMore`, and `writeEnd`.
   - Raise exceptions if encounter errors when the fetcher failed to load a resource of a given URL.
* TUNING:

## Version 0.8.0

On July 31, 2022, HVML Community announces the availability of PurC 0.8.0,
   which is also the first alpha release of PurC 1.0.x.

This version implements almost all features defined by [HVML Specifiction V1.0],
      and also implements almost all predefined dynamic variables defined by [HVML Predefined Variables V1.0].

For bugs, incompatibilities, and issues, please report to <https://github.com/HVML/PurC/issues>.

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

