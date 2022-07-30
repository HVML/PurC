# TODO List

## 0) Features not Planned yet

### 0.1) Predefined Varaibles

1. Support for the following URI schemas for `$STREAM`:
   - `fifo`
   - `tcp`
1. Support for the following filters for `$STREAM`:
   - `ssl`
   - `websocket`
   - `http`
   - `hibus`

## 1) Features Planned for Version 1.0

### 1.1) Variants

1. Support for the new variant type: tuple.
1. Use an indepedent structure to maintain the listeners of variants, so we can decrease the size of a variant structure.
1. Support for `rdrState:connLost` event on `$CRTN`.

### 1.2) eJSON and HVML Parsing and Evaluating

1. Support for prefix for foreign tag name.
1. Optimize the content evalution of foreign elements: make sure there is only one text node after evaluating the contents `$< Hello, world! --from COROUTINE-$CRTN.cid`.
1. Support for tuples.

### 1.3) Predefined Varaibles

1. Implement `$CRTN.native_crtn` method.
1. In the implementation of predefined variables, use the interfaces for linear container instead of array.
1. Finish the implementation of the following predefined variables:
   - `$RDR`
   - `$DOC`
   - `$URL`
   - `$STR`

### 1.4) eDOM

1. Optimize the implementation of element collection, and provide the support for CSS Selector Level 3.
1. Optimize the implementation of the map from `id` and `class` to element.
1. Support for the new tarege document type: `plain` and/or `markdown`.

### 1.5) Interpreter

1. Provide support for channel, which can act as an inter-coroutine communication mechanism.
1. Implement the `request` tag.
1. Support for use of an element's identifier as the value of the `at` attribute in an `init` element.
1. Provide support for `type` attribute of the element `archetype`. It can be used to specify the type of the template contents, for example, `plain`, `html`, `xgml`, `svg`, or `mathml`.
1. Improve support for the attribute `in`, so we can use a value like `> p` to specify an descendant as the current document position.
1. Improve the element `init` to make the attribute `as` is optional, so we can use `init` to initilize a data but do not bind the data to a variable.
1. Improve the element `init` and `bind` to make the attribute `at` support `_runner`, so we can create a runner-level variable.
1. Review the implementation of all elements.
1. Improve the implementation of the element `update`:
   - The value of the attribute `to` can be `prepend`, `remove`, `insertBefore`,  `insertAfter`, `intersect`, `subtract`, and `xor`.
   - The value of the attribute `at` can be `content`.
   - The support for the adverb attribute `individually`.
   - The support for the attribute `type` of the element `archetype`.
1. Improve the function to get data from remote data fetcher:
   - The element `archetype`: support for `src`, `param`, and `method` attributes.
   - The element `archedata`: support for `src`, `param`, and `method` attributes.
   - The element `execpt`: support for `src`, `param`, and `method` attributes.
   - The element `init`: support for `from`, `with`, and `via` attrigbutes.
   - The element `define`: support for `from`, `with`, and `via` attributes.
   - The element `update`: support for `from`, `with`, and `via` attributes.
1. The generation and handling mechanism of uncatchable errors:
   - Support for the element `error`.
   - The element `error`: support for `src`, `param`, and `method` attributes.

### 1.6) More ports

1. Windows

### 1.7) Others

1. Clean up all unnecessary calls of `PC_ASSERT`.
1. Raise exceptions if encounter errors when executing elements instead of aborting the process.
1. Tune `PC_ASSERT` to suppress any code when building for release.
1. Normalize the typedef names.
1. Tune API description.

