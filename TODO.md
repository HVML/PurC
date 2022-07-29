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

## 2) Features Planned for V0.8

### 2.1) 变体

（无）

### 2.2) 预定义变量

1. 增加、调整或补充预定义变量的实现：
   - `$CRTN` 上支持 `rdrState:pageClosed` 事件：协程对应的渲染器页面被用户关闭。
   - `$CRTN` 上支持 `rdrState:connLost` 事件：协程所在行者丢失渲染器的连接。
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
   - ~~合并协程栈以及协程数据结构中重复的字段。~~
   - ~~协程结构中，使用 `cid` 替代 `ident` 名称。~~
   - ~~协程结构中，使用 `uri` 替代 `fullname` 字段名（该字段其实没必要保存两份，通过 `cid` 在原子字符串中查找即可获得）。~~
   - ~~`$CRTN` 变量使用协程数据结构做初始化，并实现 `cid` 属性、`uri` 属性以及 `token` 属性获取器。~~
   - ~~实现 `$RUNNER` 变量的 `rid` 属性以及 `uri` 属性获取器。~~
   - ~~移除代码中冗余的针对 `get_coroutine` 以及 `get_stack` 的调用。~~
   - ~~将 `$L`, `$STR`, `$URL`, `$EJSON`, `$STREAM`, `$DATETIME` 调整为行者级变量。~~
1. 调整或完善相关实现：
   - ~~调整调度器实现，通过设置协程进入停止状态以及对应的唤醒条件来实现 协程同步等待数据获取器的返回、等待特定请求的响应消息、主动休眠的超时及提前唤醒、其他协程的退出状态、 并发调用的返回等。~~
   - ~~`idle` 事件：当所有协程进入事件驱动执行阶段，但未收到任何底层事件的时间累计达到或超过 100ms 时，自动产生 `idle` 事件，广播给所有正在 `$CRTN` 变体上监听 `idle` 事件的协程；`idle` 事件可规约。~~
   - ~~`purc_schedule_vdom()` 支持 `body_id` 参数。~~
   - ~~移除 `sync` 线程。~~
1. 完善如下标签的实现
   - ~~`sleep` 标签：在调度器检查到针对休眠协程的事件时，可由调度器唤醒。~~
   - ~~`call` 和 `return` 标签。并发调用时，克隆 `define` 子树构建新的 vDOM 树。~~
   - ~~`load` 和 `exit` 标签。~~
1. 调整求值逻辑：
   - ~~按照规范要求，对 `hvml` 元素内容中定义的表达式进行求值并设置对应栈帧的结果数据；参阅 HVML 规范 2.3.4 小节。~~
   - ~~取消和 `head` 及 `body` （`head` 和 `body` 均为可选框架元素）相关的约束。~~
   - ~~确保 `hvml` 栈帧始终存在（对应 `_topmost`），在执行观察者代码时，应保留该栈帧存在，该栈帧的结果数据便是整个协程的执行结果。~~
   - ~~在对动作元素的属性值求值时，若该动作元素定义有表达式内容，则在当前动作元素所在栈帧中完成求值，并将其绑定到当前栈帧的上下文变量 `$0^` 上；若动作元素支持重跑（rerun），则应该在重跑时对内容重新求值并用结果替代 `$0^`；当动作元素需要 `with` 属性值但未定义时，使用内容数据。~~
1. 生成目标文档时，过滤如下特定属性：
   - ~~`hvml` 元素的 `target` 属性。~~
   - ~~外部元素中所有属性名称以 `hvml:` 打头的属性。~~
1. 渲染器对接：
   - ~~HVML 协程对应的页面类型为 `_null` 时，不创建页面，不同步 eDOM 的更新信息。~~
   - ~~增强 PURCMC 协议，用于区分普通文本或标记文本。~~
   - ~~支持页面名称为 `_inherit` 以及 `_self` 的情形。~~

### 2.6) 其他

1. ~~`purc` 命令行工具。~~
1. 代码清理
   - ~~测试程序的许可声明~~
   - API 描述
   - 自定义类型的名称规范化（仅针对结构指针添加 `_t` 后缀）
1. 文档整理。
1. 解决现有测试用例及示例程序暴露出的缺陷：
   - ~~在构建目录下，使用 `purc` 运行 `hvml/hello-world-7.hvml`，程序终止。该程序使用了 `<init as ... from "file://{$SYS.cwd}/hvml/hello-world.json" />`。~~
   - ~~在构建目录下，使用 `purc` 运行 `hvml/hello-world-8.hvml`，从输出结果看，`observe` 元素的内容被多次求值。应仅在执行 `observe` 时求值一次。~~
   - ~~在构建目录下，使用 `purc -b` 运行 `hvml/hello-world-9.hvml`，从输出结果看，`observe` 的内容（包括 `inherit` 元素）被错误地插入到了目标文档。~~
   - ~~在构建目录下，使用 `purc -b` 运行 `hvml/hello-world-a.hvml`，从输出结果看，`<span>$?.greeting$?.name</span>` 被置换后的结果不正确，丢掉了 `$?.name`；如果使用 `<span>{$?.greeting}{$?.name}</span>` 会报解析错误。~~
   - ~~在构建目录下，使用 `purc` 运行 `hvml/hello-world-b.hvml`，未得到预期的输出结果，应该是作为 `iterate` 内容的表达式未被求值。~~
   - ~~若 HVML 代码的 hvml 关闭标签 `</hvml>` 被误写为 `<hvml>`，`purc_load_vdom_xxx()` 函数返回的 vDOM 为空，但错误信息为 Ok。~~
   - ~~`test/interpreter/test_inherit_document.cpp` 中的 EJSON 字符串生成 VCM 树之后，使用自定义 `$ARGS` 对象替代其中的子字符串生成期望结果，但生成的字符串不正确。~~


