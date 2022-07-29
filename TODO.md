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

## 2) Features Planned for V0.8

### 2.1) 变体

（无）

### 2.2) 预定义变量

1. 增加、调整或补充预定义变量的实现：
   - ~~`$CRTN` 上支持 `rdrState:pageClosed` 事件：协程对应的渲染器页面被用户关闭。~~
   - ~~将 `$STREAM` 调整为行者级变量。~~
   - ~~`$CRTN`~~
   - ~~完善 `$MATH.eval` 和 `$MATH.eval_l` 对函数及常量的支持（见预定义变量规范）。~~
   - ~~`$SYSTEM` 更名为 `$SYS`~~
   - ~~`$SESSION` 更名为 `$RUNNER`（`sid` 属性更名为 `rid`）~~
   - ~~`$HVML` 更名为 `$CRTN`~~
   - ~~`$REQUEST` 更名为 `$REQ`~~
   - ~~实现 `$EJSON.arith` 和 `$EJSON.bitwise` 方法~~
   - ~~实现 `$STR.nr_bytes` 方法~~
1. 评估并合并如下预定义变量的实现：
   - ~~`$FS`~~
   - ~~`$FILE`~~

### 2.3) eDOM

1. ~~实现 `void` 目标文档类型。~~

### 2.4) EJSON 表达式或 vDOM 解析器

1. ~~字符串中的表达式求值置换，存在将不符合变量名（或键名）规格的字符识别为变量名（或键名）的情形，从而报错。如 `The cid: $CRTN.cid<`，应置换为 `The cid: 3<`。~~
1. ~~在动作元素中对 `on`、 `with`、 `onlyif` 和 `while` 属性，使用 `on 2L` 这种指定属性值的语法（不使用等号）时，应按照 EJSON 求值，不转字符串（`on 2L` 的结果应该为 longint 类型 2）。目前被当做字符串处理。见测试用例：`test/interpreter/comp/00-fibonacci-void-temp.hvml`~~
1. ~~`hvml` 元素内容应支持常规 EJSON 表达式或复合表达式。~~
1. ~~对井号注释的初步支持：忽略 HVML 文件中，在 `<DOCTYPE ` 或者 `<!-- ` 开始前，所有以 `#` 打头的行。~~
1. ~~支持在框架元素中定义多个求值表达式或复合求值表达式；参阅 HVML 规范 2.3.4 小节。~~

### 2.5) 解释器

1. 统一使用协程消息队列处理来自变体变化、渲染器以及其他实例的事件、请求或者响应消息。
   - ~~`observe` 元素支持隐含的临时变量 `_eventName` 和 `_eventSource`。~~
   - ~~`observe` 元素 `in` 属性的处理。~~
   - ~~正确区分会话级变量及协程级变量：`observe` 可观察会话级变体（$RUNNER.myObj）上的 `change` 事件；`$SYS` 上调用修改时间、当前工作路径等，会广播更新事件（如 `change:time`）给所有实例。~~
1. 接口及实现调整：
   - ~~实现 `purc_schedule_vdom()` 替代 `purc_attach_vdom_to_renderer()`。~~
   - ~~调整 `purc_bind_document_variable()` 为 `purc_coroutine_bind_variable()`。~~
1. 实现支持多实例相关的接口：
   - ~~`purc_inst_create_or_get()`~~
   - ~~`purc_inst_schedule_vdom()`~~
   - ~~完善相关测试用例，修复相关缺陷。~~
1. 整理代码：
   - ~~在创建协程时完成协程级变量的绑定，同时处理 `$REQUEST` 的绑定。~~
