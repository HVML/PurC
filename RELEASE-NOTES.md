``` mermaid
gantt
    title PurC Release Plan
    dateFormat  YYYY-MM-DD
    section Mainline Release

    ver-0.8.0           :a1, 2021-01-19, 2022-07-30
    ver-0.8.1           :a2, 2022-07-30, 2022-08-19
    ver-0.8.2           :a3, 2022-08-19, 2022-09-30
    ver-0.9.0           :a4, 2022-09-30, 2022-10-31
    ver-0.9.2           :a5, 2022-10-31, 2022-12-01
    ver-0.9.4           :a6, 2022-12-01, 2022-12-30
    ver-0.9.5           :a7, 2022-12-30, 2023-01-09
    ver-0.9.6           :a8, 2023-01-09, 2023-02-23
    ver-0.9.7           :a9, 2023-02-23, 2023-03-15
    ver-0.9.8            :a10, 2023-03-15, 2023-03-31
    ver-0.9.10           :a11, 2023-03-31, 2023-04-26
    ver-0.9.12           :a12, 2023-04-26, 2023-06-01
    ver-0.9.13           :a13, 2023-06-01, 2023-06-29
    ver-0.9.14           :a14, 2023-06-29, 2023-07-28
    ver-0.9.15           :a15, 2023-07-28, 2023-09-01
```

# Release Notes

- [Version 0.9.22](#version-0922)
- [Version 0.9.20](#version-0920)
- [Version 0.9.19](#version-0919)
- [Version 0.9.18](#version-0918)
- [Version 0.9.17](#version-0917)
- [Version 0.9.16](#version-0916)
- [Version 0.9.15](#version-0915)
- [Version 0.9.14](#version-0914)
- [Version 0.9.13](#version-0913)
- [Version 0.9.12](#version-0912)
- [Version 0.9.10](#version-0910)
- [Version 0.9.8](#version-098)
- [Version 0.9.6](#version-096)
- [Version 0.9.5](#version-095)
- [Version 0.9.4](#version-094)
- [Version 0.9.2](#version-092)
- [Version 0.9.0](#version-090)
- [Version 0.8.2](#version-082)
- [Version 0.8.0](#version-080)

## Version 0.9.22

On May 31, 2025, HVML Community announces the availability of PurC 0.9.22,
   which is also the 17th alpha release of PurC 1.0.x.

For bugs, incompatibilities, and issues, please report to <https://github.com/HVML/PurC/issues>.

### What's new in version 0.9.22

In this version, we fixed a few bugs and made some major enhancements:

* CHANGES:
   - Remove `ssl_cert` and `ssl_key` fields from `struct purc_instance_extra_info`.
   - Use new implementation of dvobjs-based socket connection to the renderer.
   - Use communication method socket for both UNIX/local socket and websocket socket, and remove obsolete code for websocket connection.
* ENHANCEMENTS:
   - Add two new exceptions: ProtocolViolation and TLSFailure.
   - Complete implementation of `$STREAM`: enhance the extension protocol `websocket` for stream socket and provide support for SSL/TLS.
   - Enhance or tune for xGUI.
   - Implement `$SOCKET.stream()` and `$SOCKET.dgram()` methods for stream socket and datagram socket respectively.
   - Implement `$SOCKET.accept()` method.
   - Implement `$dgramSocket.sendto()` and `$dgramSocket.recvfrom()`.
   - Implement `$SYS.pipe()` method.
   - Implement `$SYS.close()` method.
   - Implement `$SYS.fdflags()` method.
   - Implement `$SYS.sockopt()` property.
   - Implement `$SYS.spawn()` method.
   - Implement `$SYS.access()` method.
   - Implement `$SYS.remove()` method.
   - Implement `$stream.fd()` property.
   - Implement `$stream.peerAddr` property.
   - Implement `$stream.peerPort` property.
   - Implement `$DATA.makebytesbuffer()` method.
   - Implement `$DATA.append2bytesbuffer()` method.
   - Implement `$DATA.rollbytesbuffer()` method.
   - Implement `$STREAM.readbytes2buffer()` method.
   - Implement `$RUNNER.enablelog()` method.
   - Implement `$RUNNER.logmsg()` method.
   - Implement `$DATA.key()` method.
   - Implement `$STR.trim()` method.
   - Implement `$STR.strstr()` method.
   - Refactor `$STR.explode()`, `$STR.implode()`, `$STR.format_c()`, and `$STR.replace()` methods.
   - Enhance `$STREAM.readlines()` to support the customized line seperator.
   - Rename `$DATA.size()` to `$DATA.memsize()`.
* OPTIMIZATIONS:
   - Tune prototype of `on_message` for message/websocket protocol to reduce the memory use.
* ADJUSTMENTS:
   - Adjust the algorithm to cast a byte sequence to an integer or a float number.
   - Change the manner of `purc_inst_ask_to_shutdown()` from sending request to posting request (noreturn).
   - Support implicity variables `_observedAgainst`, `_observedOn`, `_observedFor`, `_observedWith`, `_observedContent` for `observer` element.
* BUGFIXES:
   - Fix some memory leaks and race condition crashes.
   - Fix a bug in `purc_variant_cast_to_ulongint()`: 0.0 can not be cast to ulongint.
* CLEANUP:
   - Use `snprintf()` instead of `sprintf()`.
* SAMPLES:

## Version 0.9.20

On May 31, 2024, HVML Community announces the availability of PurC 0.9.20,
   which is also the 16th alpha release of PurC 1.0.x.

For bugs, incompatibilities, and issues, please report to <https://github.com/HVML/PurC/issues>.

### What's new in version 0.9.20

In this version, we fixed a few bugs and made some major enhancements:

* CHANGES:
* ENHANCEMENTS:
   - Enhance and expose new APIs for `purc-fetcher` module.
* OPTIMIZATIONS:
* ADJUSTMENTS:
* BUGFIXES:
   - Fix some minor bugs.
   - Fix some memory leaks.
* CLEANUP:
* SAMPLES:

## Version 0.9.19

On Dec. 30, 2023, HVML Community announces the availability of PurC 0.9.19,
   which is also the 16th alpha release of PurC 1.0.x.

For bugs, incompatibilities, and issues, please report to <https://github.com/HVML/PurC/issues>.

### What's new in version 0.9.19

In this version, we fixed a few bugs and made some major enhancements:

* CHANGES:
* ENHANCEMENTS:
   - Support for a new predefined variable: `SQLite`.
* OPTIMIZATIONS:
* ADJUSTMENTS:
* BUGFIXES:
   - Fix some minor bugs.
   - Fix some memory leaks.
* CLEANUP:
* SAMPLES:

## Version 0.9.18

On Dec. 7, 2023, HVML Community announces the availability of PurC 0.9.18,
   which is also the 15th alpha release of PurC 1.0.x.

For bugs, incompatibilities, and issues, please report to <https://github.com/HVML/PurC/issues>.

### What's new in version 0.9.18

In this version, we fixed a few bugs and made some major enhancements:

* CHANGES:
* ENHANCEMENTS:
   - Update WTF to use libsoup-3.0; use `-DUSE_SOUP2=ON` if you want use libsoup-2.x.
   - Support for PURCMC version 160.
   - Add a new renderer `seeker` for seeking a real available remote renderer.
   - Support for switching amone renderers.
   - Add `transitionStyle` and `layoutStyle` for windows.
   - Support for app menifest.
   - Support for `template` attribute of `hvml` tag.
   - Add a new API: `purc_evaluate_standalone_window_transition_from_styles()`.
   - Add a new API: `purc_evaluate_standalone_window_geometry_from_styles()`.
   - Add a new API: `purc_split_page_identifier()`.
   - Add a new API: `purc_get_app_icon_content()`.
   - Add a new API: `purc_get_app_icon_url()`.
* OPTIMIZATIONS:
* ADJUSTMENTS:
* BUGFIXES:
   - Fix some minor bugs.
   - Fix some memory leaks.
* CLEANUP:
* SAMPLES:

## Version 0.9.17

On Nov. 1, 2023, HVML Community announces the availability of PurC 0.9.17,
   which is also the 14th alpha release of PurC 1.0.x.

For bugs, incompatibilities, and issues, please report to <https://github.com/HVML/PurC/issues>.

### What's new in version 0.9.17

In this version, we fixed a few bugs and made some minor enhancements:

* CHANGES:
   - Add a new feature: `PURC_FEATURE_APP_AUTH`.
   - Refactor `purc_is_feature_enabled()`.
   - Add new API `purc_document_type()`.
* ENHANCEMENTS:
   - Support for WebSocket in PURCMC implementation and `$STREAM`.
* OPTIMIZATIONS:
* ADJUSTMENTS:
* BUGFIXES:
   - Fix some minor bugs.
* CLEANUP:
* SAMPLES:

## Version 0.9.16

On Otc. 1, 2023, HVML Community announces the availability of PurC 0.9.16,
   which is also the 14th alpha release of PurC 1.0.x.

For bugs, incompatibilities, and issues, please report to <https://github.com/HVML/PurC/issues>.

### What's new in version 0.9.16

In this version, we fixed a few bugs and made some minor enhancements:

* CHANGES:
* ENHANCEMENTS:
   - Support two new element types in PURCMC message: `PCRDR_MSG_ELEMENT_TYPE_CLASS` and `PCRDR_MSG_ELEMENT_TYPE_TAG`.
   - Support `.class-name` and `tag-name` in `request` and `observe` elements.
   - Create HTML document with explicit DOCTYPE.
   - Implement `receivable` event on channels.
* OPTIMIZATIONS:
* ADJUSTMENTS:
   - Update HVML samples to use bootstrap 5.3.1 and bootstrap-icons 1.10.5.
* BUGFIXES:
   - Fix some minor bugs.
* CLEANUP:
* SAMPLES:

## Version 0.9.15

On Sept. 1, 2023, HVML Community announces the availability of PurC 0.9.15,
   which is also the 13th alpha release of PurC 1.0.x.

For bugs, incompatibilities, and issues, please report to <https://github.com/HVML/PurC/issues>.

### What's new in version 0.9.15

In this version, we fixed a few bugs and made some minor enhancements:

* CHANGES:
* ENHANCEMENTS:
* OPTIMIZATIONS:
* ADJUSTMENTS:
   - Call `pcintr_rdr_page_control_load` for inherit page;
* BUGFIXES:
   - Support for `at` attribute value `_topmost` in `init`.
   - Fix the request parameters are not passed (HTTP GET).
   - Other minor bugs.
* CLEANUP:
   - Cleanup cmake files for deprecated usage.
   - Add executable permission to .py files.
   - Merge some modifications from @taotieren.
* SAMPLES:

## Version 0.9.14

On July 31, 2023, HVML Community announces the availability of PurC 0.9.14,
   which is also the 12th alpha release of PurC 1.0.x.

For bugs, incompatibilities, and issues, please report to <https://github.com/HVML/PurC/issues>.

### What's new in version 0.9.14

In this version, we fixed a few bugs and made some minor enhancements:

* CHANGES:
* ENHANCEMENTS: Tune or enhance the implementation according to the revisions from RC9 to RCd of HVML Spec V1.0:
   - The default result data of a foreign element: inheriting from the proceeding operation.
   - Tune the manners of `test`, `match`, `differ` elements and fix known bugs.
   - Tune the manners of `back` and `update` elements and fix known bugs.
   - Tune the manners of `update` elements to reflect the `wholly` adverb attribute.
   - The result data of a `catch` elmenet, should be defined as an object to describe the exception.
   - Enhance `init` element to support `RAW-HEADERS` when issuing an HTTP request to get data from a remote URL.
   - Enhance `request` element to pass CSS selector directly to the renderer instead of passing the HVML handle of the element.
* OPTIMIZATIONS:
* ADJUSTMENTS:
* BUGFIXES:
   - Fix bugs in `$FS.file_contents` getter and setter.
* CLEANUP:
* SAMPLES:

## Version 0.9.13

On June 30, 2023, HVML Community announces the availability of PurC 0.9.13,
   which is also the 11th alpha release of PurC 1.0.x.

For bugs, incompatibilities, and issues, please report to <https://github.com/HVML/PurC/issues>.

### What's new in version 0.9.13

In this version, we fixed a few bugs and made some minor enhancements:

* CHANGES:
   - If `$REQ` is not defined, make it as an empty object.
* ENHANCEMENTS:
   - Implement `$DATA.is_divisible`, `$DATA.match_members`, and `$DATA.match_properties`.
* OPTIMIZATIONS:
* ADJUSTMENTS:
* BUGFIXES:
   - Fix bugs in `$FS.file_contents` getter and setter.
* CLEANUP:
* SAMPLES:

## Version 0.9.12

On May 31, 2023, HVML Community announces the availability of PurC 0.9.12,
   which is also the 10th alpha release of PurC 1.0.x.

For bugs, incompatibilities, and issues, please report to <https://github.com/HVML/PurC/issues>.

### What's new in version 0.9.12

In this version, we fixed some bugs and made some enhancements:

* CHANGES:
   - Change API `purc_enable_with_tag()` to specify the log level.
   - Expose `pcutils_md5_xxx()` and `pcutils_sha1_xxx()` APIs.
   - Expose `pcutils_printbuf_xxx()` APIs.
* ENHANCEMENTS:
   - Add new APIs: `pcutils_sha256_xxx()`, `pcutils_sha512_xxx()`, and `pcutils_hmac_sha256()`.
   - Add new APIs: `purc_enable_log_ex()` and `purc_log_with_level()`, and use level mask to control the message levels to log ultimately.
   - Enhance `$STREAM` to support `message` protocol on UNIX-domain socket and `HBDBus` data bas protocol.
* OPTIMIZATIONS:
* ADJUSTMENTS:
* BUGFIXES:
   - Remove some limitations on event name.
* CLEANUP:
* SAMPLES:

## Version 0.9.10

On Apr. 30, 2023, HVML Community announces the availability of PurC 0.9.10,
   which is also the nineth alpha release of PurC 1.0.x.

For bugs, incompatibilities, and issues, please report to <https://github.com/HVML/PurC/issues>.

### What's new in version 0.9.10

In this version, we fixed some bugs and made some enhancements:

* CHANGES:
   - Expose `pcutils_kvslist_xxx` interfaces to applications.
   - Implement `$CRTN.static` and `$CRTN.temp` to help HVML program to access or operate the static or temporary variables in HEE.
* ENHANCEMENTS:
   - Support for PURCMC 120 protocol.
   - Tune the parsing errors of HVML and HEE.
   - Support for the daynamic update of document in Foil renderer.
   - Add new text cases for Foil renderer.
* OPTIMIZATIONS:
* ADJUSTMENTS:
* BUGFIXES:
   - Fix bugs in implementation of `match` and `back` elements.
   - Fix bugs in implementation of event name.
* CLEANUP:
* SAMPLES:
   - New sample `file-system-browser.hvml`: using the same renderer page among multiple coroutines.

## Version 0.9.8

On Mar. 31, 2023, HVML Community announces the availability of PurC 0.9.8,
   which is also the seventh alpha release of PurC 1.0.x.

For bugs, incompatibilities, and issues, please report to <https://github.com/HVML/PurC/issues>.

### What's new in version 0.9.8

In this version, we fixed some bugs and made some enhancements:

* CHANGES:
   - Rename `purc_variant_is_mutable()` to `purc_variant_is_container()`.
   - Use `purc_ejson_parsing_tree` instead of `purc_ejson_parse_tree`; use `purc_ejson_parse_` instead of `purc_variant_ejson_parse_`.
   - Add a new API: `purc_variant_object_set_by_ckey()`.
   - Rename `purc_variant_object_remove_by_static_ckey()` to `purc_variant_object_remove_by_ckey()`.
   - Add a new API: `purc_variant_set_unique_keys()`. Use this function to get the unique keys of a variant set.
   - Add a new API: `purc_variant_make_native_entity()`. Use this function to create a native entity with name. Note that `purc_variant_make_native()` will create a native entity named `anonymous`.
   - Add a new API: `purc_get_elapsed_milliseconds()`. Use this function to get the elapsed milliseconds since the specified time.
   - Add a new API: `pcutils_mystring_append_uchar()`. Use this function to append a Unicode character to the mystring object.
   - Add a new API: `purc_make_object_from_query_string()`.
   - Add new APIs: `pcutils_utf8_to_unichar()`, `pcutils_string_utf8_chars_with_nulls()`, and `pcutils_string_decode_utf8_alloc_with_nulls()`.
   - Add a new generic error: `PURC_ERROR_IO_FAILURE`.
* ENHANCEMENTS:
   - The basic implementation of `$PY`. HVML now can interact with Python.
   - Draw borders of boxes in Foil renderer.
   - Support for floats and absoluted positioning in Foil renderer.
   - Add a new exception `ExternalFailure` for errors in external dynamic variant objects.
   - The native entity now supports getter and setter on itself.
   - Add a new post listener operation: `PCVAR_OPERATION_RELEASING`.
   - Add a auto-test HVML program and many test cases to test Foil renderer.
* OPTIMIZATIONS:
* ADJUSTMENTS:
* BUGFIXES:
   - Fix some bugs related timezone (from @bkmgit).
   - Fix lost of event `rdrState:pageClosed`.
* CLEANUP:
   - Remove repeated identical test (from @bkmgit)
* SAMPLES:
   - A new sample `embed-python-looking-for-primes.hvml`: Embedding Python in HVML to find primes.
   - A new sample `embed-python-animated-3d-random-walk.hvml`: Embedding Python in HVML to show the animated 3D random walks.
   - A new sample `foil-three-columns.hvml`: Using Foil to show three columns to run samples.

## Version 0.9.6

On Feb. 25, 2023, HVML Community announces the availability of PurC 0.9.6,
   which is also the sixth alpha release of PurC 1.0.x.

For bugs, incompatibilities, and issues, please report to <https://github.com/HVML/PurC/issues>.

### What's new in version 0.9.6

In this version, we fixed some bugs and made some enhancements:

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

In this version, we fixed some bugs and made some enhancements:

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

In this version, we fixed some bugs and made some enhancements:

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

In this version, we fixed some bugs and made some enhancements:

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

In this version, we fixed some bugs and made some enhancements:

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

In this version, we fixed some bugs and made some enhancements:

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

