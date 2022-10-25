# TODO List

**Table of Contents**

[//]:# (START OF TOC)

- [1) Features not Planned yet](#1-features-not-planned-yet)
   + [1.1) Syntax Highlighting Support for Various Editors.](#11-syntax-highlighting-support-for-various-editors)
   + [1.2) Predefined Variables](#12-predefined-variables)
   + [1.3) HVML Debugger](#13-hvml-debugger)
- [2) Features Planned for Version 1.0](#2-features-planned-for-version-10)
   + [2.1) Variants](#21-variants)
   + [2.2) eJSON and HVML Parsing and Evaluating](#22-ejson-and-hvml-parsing-and-evaluating)
   + [2.3) Predefined Variables](#23-predefined-variables)
   + [2.4) eDOM](#24-edom)
   + [2.5) Interpreter](#25-interpreter)
   + [2.6) `purc`](#26-purc)
   + [2.7) More Platforms](#27-more-platforms)
   + [2.8) Others](#28-others)
   + [2.9) Known Bugs](#29-known-bugs)

[//]:# (END OF TOC)


## 1) Features not Planned yet

### 1.1) Syntax Highlighting Support for Various Editors.

1. vim
1. vscode
1. gitlab
1. github

### 1.2) Predefined Variables

1. Support for the following URI schemas for `$STREAM`:
   - `tcp`
1. Support for the following filters for `$STREAM`:
   - `http`
   - `ssl`
   - `websocket`
   - `mqtt`

### 1.3) HVML Debugger

## 2) Features Planned for Version 1.0

### 2.1) Variants

1. [0.9.0] Basic support for the new variant type: tuple.
1. [0.9.2] Full support for the new variant type: tuple.
1. [0.9.4] Use an indepedent structure to maintain the listeners of variants, so we can decrease the size of a variant structure.

### 2.2) eJSON and HVML Parsing and Evaluating

1. [0.8.2; Resolved] Support for prefix for foreign tag name. See Section 3.1.1 of [HVML Specifiction V1.0].
1. [0.8.2; Resolved] Support for using Unihan characters in variable names and property/method names. See Section 2.2.2 of [HVML Specifiction V1.0].
1. [0.8.2; Resolved] Optimize the content evaluation of foreign elements: make sure there is only one text node after evaluating the contents `$< Hello, world! --from COROUTINE-$CRTN.cid`.
1. [0.8.2; Resolved] Improve eJSON parser to support the following string patterns:
   - `file://$SYS.cwd/a.hvml`
   - `$SYS.time,$SYS.time~$SYS.time-$SYS.time@$SYS.time#$SYS.time%$SYS.time^$SYS.time&$SYS.time*$SYS.time+$SYS.time=$SYS.time\$SYS.time|$SYS.time>$SYS.time:$SYS.time!$SYS.time<$SYS.time?$SYS.time;`
   - `$SYS.time；$SYS.time、$SYS.time　$SYS.timeａ$SYS.time。$SYS.time，$SYS.time“$SYS.time”$SYS.time`
1. [0.8.2; Resolved] Keep self-closed foreign elements not changed.
1. [0.9.2] Support line comments in CJSONEE.
1. [0.9.2] Support for tuples.

### 2.3) Predefined Variables

1. [0.8.2; Resolved] Implement `$RUNNER.chan` and the native entity representing a channel, which can act as an inter-coroutine communication (ICC) mechanism. See Section 3.2.7 of [HVML Predefined Variables V1.0].
1. [0.8.2; Resolved] Tune `$SYS.sleep` to utilize evaluating again.
1. [0.9.2] In the implementation of predefined variables, use the interfaces for linear container instead of array.
1. [0.9.5] Complete the implementation of the following predefined variables:
   - `$RDR`
   - `$DOC`
   - `$URL`
   - `$STR`
1. [0.9.9] Support for the following URI schemas for `$STREAM`:
   - `fifo`
1. [0.9.9] Support for the following filters for `$STREAM`:
   - `hibus`
   - `gzip`

### 2.4) eDOM

1. [0.9.2] Optimize the implementation of element collection, and provide the support for CSS Selector Level 3.
1. [0.9.2] Optimize the implementation of the map from `id` and `class` to element.
1. [Pending] Support for the new target document type: `plain` and/or `markdown`.

### 2.5) Interpreter

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
1. [0.9.0] Support for the equivalences and/or abbreviations of some adverb attributes.
1. [0.9.0; Resolved] Support for the new preposition attribute: `idd-by`.
1. [0.9.0; Pending] Improve the element `bind` to make the attribute `at` support `_runner`, so we can create a runner-level variable.
1. [0.9.2] Full support for `request` element.
1. [0.9.2] Optimize and improve the synchronization between eDOM and uDOM when multiple coroutines share the same eDOM.
1. [0.9.2] Improve the implementation of the element `update`:
   - The value of the attribute `to` can be `intersect`, `subtract`, and `xor`.
   - The value of the attribute `at` can be `content`.
   - The support for the adverb attribute `individually`.
1. [0.9.2] Improve support for the attribute `in`, so we can use a value like `> p` to specify an descendant as the current document position.
1. [0.9.5] The generation and handling mechanism of uncatchable errors:
   - Support for the element `error`.
   - The element `error`: support for `src`, `param`, and `method` attributes.
1. [0.9.5] Review the implementation of all elements.
1. [0.9.5] Review all uses of exception names.
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

1. [0.9.0] A simple built-in HTML renderer for text terminal via `THREAD` protocol.
1. [0.9.2] A full-featured and full-screen built-in HTML renderer for text terminal via `THREAD` protocol.

### 2.7) More Platforms

1. [1.0.0] Windows

### 2.8) Others

1. [0.9.0; Resolved] Merge repo of `DOM Ruler` to `PurC` (under `Source/CSSEng` and `Source/DOMRuler`).
1. [0.9.0] Merge repo of `PurC Fetcher` to `PurC` (under `Source/RemoteFetcher`).
1. [1.0.0] Clean up all unnecessary calls of `PC_ASSERT`.
1. [1.0.0] Normalize the typedef names.
1. [1.0.0] Rewrite the code fragments in coding pattern `do { if (...) break; } while (0)` in source files:
    We should only use this pattern when defining macros or just creating a temp. variable scope, because this coding pattern seriously reduces code readability.
1. [1.0.0] Tune API description.
1. [0.8.1; Resolved] Tune `PC_ASSERT` to suppress any code when building for release.

### 2.9) Known Bugs

1. [0.8.2; Resolved] The condition handler will get `PURC_COND_COR_EXITED` after got `PURC_COND_COR_TERMINATED`.
1. [0.8.2; Resolved] When dumping the stacks, we should use the writing order the attributes of an element, not the sorted order.
1. [0.8.2; Resolved] Some requests to renderer might be sent twice.
1. [0.8.1; Resolved] The content of an `iterate` element may be evaluated twice.
1. [0.8.1; Resolved] The samples with bad result:
   - Incorrect evaluation logic of a CJSONEE with `&&` and `||`.
   - `hvml/greatest-common-divisor.hvml`: Adjust the evaluating logic of CJSONSEE.
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

