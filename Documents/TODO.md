# TODO List

**Table of Contents**

[TOC]

## 1) Features not Planned yet

### 1.1) Syntax Highlighting Support for Various Editors.

1. vim
1. vscode
1. gitlab
1. github

### 1.2) HVML Debugger

## 2) Features Planned for Version 1.0

### 2.1) Variants

1. [0.9.6; Resolved] Optimize variant module.
1. [0.9.2; Resolved] Full support for the new variant type: tuple.
1. [0.9.2; Resolved] Implement some new APIs:
   - `purc_variant_make_atom()`
1. [0.9.0; Resolved] Basic support for the new variant type: tuple.
1. [Pending] Use an indepedent structure (hash table) to maintain the listeners of variants, so we can decrease the size of a variant structure.

### 2.2) HEE and HVML Parsing and Evaluating

1. [0.9.10; Resolved] Optimize the HVML and HEE parsing error messages.
1. [0.9.6; Resolved] Optimize the evaluation of a VCM tree to decrease uses of `malloc()` and `free()`.
1. [0.9.2; Resolved] Provide support for triple-single-qutoe syntax and make sure the HEEs which are embedded in a string enclosed by two triple-double-quotes be evaluated.
1. [0.9.2; Resolved] Support line comments in CHEE.
1. [0.9.2; Resolved] Support for tuples.
1. [0.9.2; Resolved] Support for string constants.
1. [0.8.2; Resolved] Support for prefix for foreign tag name. See Section 3.1.1 of [HVML Specifiction V1.0].
1. [0.8.2; Resolved] Support for using Unihan characters in variable names and property/method names. See Section 2.2.2 of [HVML Specifiction V1.0].
1. [0.8.2; Resolved] Optimize the content evaluation of foreign elements: make sure there is only one text node after evaluating the contents `$< Hello, world! --from COROUTINE-$CRTN.cid`.
1. [0.8.2; Resolved] Improve eJSON parser to support the following string patterns:
   - `file://$SYS.cwd/a.hvml`
   - `$SYS.time,$SYS.time~$SYS.time-$SYS.time@$SYS.time#$SYS.time%$SYS.time^$SYS.time&$SYS.time*$SYS.time+$SYS.time=$SYS.time\$SYS.time|$SYS.time>$SYS.time:$SYS.time!$SYS.time<$SYS.time?$SYS.time;`
   - `$SYS.time；$SYS.time、$SYS.time　$SYS.timeａ$SYS.time。$SYS.time，$SYS.time“$SYS.time”$SYS.time`
1. [0.8.2; Resolved] Keep self-closed foreign elements not changed.

### 2.3) Predefined Variables

1. [0.9.24] Complete the implementation of the following predefined variables:
   - `$URL`
   - `$STR`
   - `$DATA`
1. [0.9.22; Resolved] Support for the following URI schemas for `$STREAM`:
   - `inet`
   - `fifo`
1. [0.9.22; Resolved] Support for `$SOCKET`.
1. [0.9.22; Resolved] Support for the following extension protocols for `$STREAM` and `$SOCKET`:
   - `websocket`
   - `hbdbus`
1. [0.9.10; Resolved] Implementation of `$CRTN.static` and `$CRTN.temp`.
1. [0.9.8; Resolved] Basic support for `$PY`.
1. [0.9.6; Resolved] Complete the implementation of the following predefined variables:
   - `$DOC` (including the document entity and the element collection entity).
1. [0.9.2; Resolved] In the implementation of predefined variables, use the interfaces for linear container instead of array.
1. [0.8.2; Resolved] Implement `$RUNNER.chan` and the native entity representing a channel, which can act as an inter-coroutine communication (ICC) mechanism. See Section 3.2.7 of [HVML Predefined Variables V1.0].
1. [0.8.2; Resolved] Tune `$SYS.sleep` to utilize evaluating again.

### 2.4) eDOM

1. [0.9.6; Resolved] Optimize the implementation of element collection, and provide the support for CSS Selector Level 3.
1. [0.9.6; Resolved] Optimize the implementation of the map from `id` and `class` to element(s).
1. [Pending] Enhance `purc_document` to fast validate an HVML handle.
1. [Pending] Support for the new target document type: `plain` and/or `markdown`.

### 2.5) Interpreter

1. [0.9.30] The generation and handling mechanism of uncatchable errors:
   - Support for the element `error`.
   - The element `error`: support for `src`, `param`, and `method` attributes.
1. [0.9.30] Review all uses of exception names.
1. [0.9.30] Review the implementation of all elements.
1. [0.9.22] Support for new renderer events: `rdrState:pageActivated` and `rdrState:pageDeactivated`.
1. [0.9.14; Resolved] Tune or enhance the implementation according to the revisions from RC9 to RCd of HVML Spec V1.0:
   - The default result data of a foreign element: inheriting from the proceeding operation.
   - Tune the manners of `test`, `match`, `differ` elements and fix known bugs.
   - Tune the manners of `back` and `update` elements and fix known bugs.
   - Tune the manners of `update` elements to reflect the `wholly` adverb attribute.
   - The result data of a `catch` elmenet, should be defined as an object to describe the exception.
   - Enhance `init` element to support `RAW-HEADERS` when issuing an HTTP request to get data from a remote URL.
   - Enhance `request` element to pass CSS selector directly to the renderer instead of passing the HVML handle of the element.
1. [0.9.10; Resolved] Tune the implementation related to PURCMC protocol to support plainwin/widget management.
1. [0.9.6; Resolved] Improve support for the attribute `in`, so we can use a value like `> p` to specify an descendant as the current document position.
1. [0.9.6; Resolved] Improve `init` to load content with MIME type `text/html` as a document entity.
1. [0.9.4; Resolved] Improve the implementation of the element `update`:
   - The value of the attribute `to` can be `intersect`, `subtract`, and `xor`.
   - The value of the attribute `at` can be `content`.
   - The support for the adverb attribute `individually`.
1. [0.9.4; Resolved] Full support for `request` element.
1. [0.9.0; Resolved] Improve the implementation of the element `bind`:
   - The support for the adverb attribute `constantly`.
   - The support for the substituting expression.
1. [0.9.0; Resolved] Improve the element `init` to make the attribute `at` support `_runner`, so we can create a runner-level variable.
1. [0.9.0; Resolved] Improve the data fetcher to generate the progress events when fetching data.
1. [0.9.0; Resolved] Improve the function to get data from remote data fetcher:
   - The element `archetype`: support for `src`, `param`, and `method` attributes.
   - The element `archedata`: support for `src`, `param`, and `method` attributes.
   - The element `execpt`: support for `src`, `param`, and `method` attributes.
   - The element `init`: support for `from`, `with`, and `via` attrigbutes.
   - The element `define`: support for `from`, `with`, and `via` attributes.
   - The element `update`: support for `from`, `with`, and `via` attributes.
1. [0.9.0; Resolved] Support for the equivalence of the context variable `<`: `~`.
1. [0.9.0; Resolved] Support for the equivalences and/or abbreviations of some adverb attributes.
1. [0.9.0; Resolved] Support for the new preposition attribute: `idd-by`.
1. [0.9.0; Pending] Improve the element `bind` to make the attribute `at` support `_runner`, so we can create a runner-level variable.
1. [0.8.2; Resolved] Enhance the evaluation of VCM to support `PURC_ERROR_AGAIN`.
1. [0.8.2; Resolved] Enhance scheduler to support support `PURC_ERROR_AGAIN`.
1. [0.8.2; Resolved] Raise an exception for a failed evaluation of an eJSON expression.
1. [0.8.2; Resolved] Enhance `purc_coroutine_dump_stack` to show the specific failed position, e.g., a call to a getter or setter with arguments, when an uncaught exception raised.
1. [0.8.2; Resolved] The support for the attribute `type` of the element `archetype`.
1. [0.8.1; Resolved] Support for `rdrState:connLost` event on `$CRTN`.
1. [0.8.1; Resolved] Implement new APIs: `purc_coroutine_dump_stack`.
1. [0.8.1; Resolved] Support for use of an element's identifier as the value of the `at` attribute in an `init` element.
1. [0.8.1; Resolved] Improve the element `init` to make the attribute `as` is optional, so we can use `init` to initilize a data but do not bind the data to a variable.
1. [0.8.1; Resolved] Implement the `request` tag (only inter-coroutine request).
1. [0.8.1; Resolved] Provide support for `type` attribute of the element `archetype`. It can be used to specify the type of the template contents, for example, `plain`, `html`, `xgml`, `svg`, or `mathml`.

### 2.6) `purc`

1. [0.9.18; Moved to xGUI] Enhance Foil to support table.
1. [0.9.18; Moved to xGUI] Enhance Foil to support flex and grid layout.
1. [0.9.18; Moved to xGUI] Enhance Foil to support full-screen, controls, and multiple-page rendering.
1. [0.9.10; Resolved] Enhance Foil to support dynamic change of element styles and contents.
1. [0.9.8; Resolved] Enhance Foil to render borders.
1. [0.9.8; Resolved] Enhance Foil to support floats and absolute positioning.
1. [0.9.6; Resolved] Enhance Foil to support `meter` and `progress` elements.
1. [0.9.4; Resolved] Enhance Foil to support more properties and layouts except for table.
1. [0.9.0; Resolved] A simple built-in HTML renderer (Foil) for text terminal via `THREAD` protocol.

### 2.7) More Platforms

1. [0.9.20] Windows

### 2.8) Others

1. [0.9.30] Clean up all unnecessary calls of `PC_ASSERT`.
1. [0.9.30] Normalize the typedef names.
1. [0.9.30] Rewrite the code fragments in coding pattern `do { if (...) break; } while (0)` in source files:
    We should only use this pattern when defining macros or just creating a temp. variable scope, because this coding pattern seriously reduces code readability.
1. [0.9.30] Tune API description.
1. [0.9.0; Resolved] Merge repo of `DOM Ruler` to `PurC` (under `Source/CSSEng` and `Source/DOMRuler`).
1. [0.9.0; Resolved] Merge repo of `PurC Fetcher` to `PurC` (under `Source/RemoteFetcher`).
1. [0.8.1; Resolved] Tune `PC_ASSERT` to suppress any code when building for release.

### 2.9) Known Bugs and Test Cases

1. [0.9.4; Resolved] Add new test cases or enhance test cases for newly found bugs.
1. [0.9.4; Resolved] `Source/test/interpreter/03-again-sleep.hvml`: the child coroutine only writes 5 lines, but 6 lines expected.
1. [0.9.4; Resolved] `Source/Sample/hvml/calculator-bc.hvml` will cause `purc` core dump if there is no `bc` installed in the system.
1. [0.9.2; Resolved] Add new test cases for following new bugs.
1. [0.9.2; Resolved] Check eJSON parser for C0 control characters in strings.
1. [0.9.2; Resolved] Crash if the content of a foreign element contains a `(` character.
1. [0.9.2; Resolved] The exrepssions in `hvml` might be evaluated twice.
1. [0.9.2; Resolved] Keep all whitespaces in text content of a foreign element, but the leading whitespaces are removed now.
1. [0.9.2; Resolved] There is an odd call to writelines() in `Source/test/interpreter/comp/31-again-channel-reader-writer.hvml`.
1. [0.8.2; Resolved] The condition handler will get `PURC_COND_COR_EXITED` after got `PURC_COND_COR_TERMINATED`.
1. [0.8.2; Resolved] When dumping the stacks, we should use the writing order the attributes of an element, not the sorted order.
1. [0.8.2; Resolved] Some requests to renderer might be sent twice.
1. [0.8.1; Resolved] The content of an `iterate` element may be evaluated twice.
1. [0.8.1; Resolved] The samples with bad result:
   - Incorrect evaluation logic of a CHEE with `&&` and `||`.
   - `hvml/greatest-common-divisor.hvml`: Adjust the evaluating logic of CHEE.
   - `hvml/hello-world-c-bad.hvml`: `$0<) Helo, world! -- from HVML COROUTINE # $CRTN.cid"`; expected: `0) Helo, world! -- from HVML COROUTINE # $CRTN.cid`; but got `0`.
1. [0.8.1; Resolved] Improve eJSON parser to support the following patterns:
   - `$?.greating$?.name`: `Hello, Tom`
   - `$?.greating{$?.name}`: `Hello, Tom`
   - `{$?.greating}{$?.name}`: `Hello, Tom`
   - `$?.greating<any CHAR not a valid variable token character>`: `Hello<any CHAR not a valid variable token character>`
   - `${output_$CRTN.target}`: `$output_html` or `$output_void`; the evaluated result in `{ }` should be a string and a valid variable token.
1. [0.8.1; Resolved] Raise exceptions if encounter errors when the fetcher failed to load a resource of a given URL.
1. [0.8.1; Resolved] When the contents of the target document is very large, send the contents by using operations `writeBegin`, `writeMore`, and `writeEnd`.


[HVML Specifiction V1.0]: https://github.com/HVML/hvml-docs/blob/master/zh/hvml-spec-v1.0-zh.md
[HVML Predefined Variables V1.0]: https://github.com/HVML/hvml-docs/blob/master/zh/hvml-spec-predefined-variables-v1.0-zh.md

