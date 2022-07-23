# TODO List

## 0) 长期

### 0.1) 变体

1. 实现新的线性容器类型元组（tuple）。
1. 使用独立的数据结构处理变体的监听器等信息，从而降低变体数据结构的尺寸。

### 0.2) eJSON 解析和求值

1. 支持元组。

### 0.3) 预定义变量

1. 在预定义变量的实现中，使用线性容器封装接口获取容器类参数的大小及其成员。
1. 完善如下变量的实现：
   - `$RDR`
   - `$DOC`
   - `$URL`
   - `$STR`

### 0.2) eDOM

1. 优化元素汇集原生实体的实现，使之符合 CSS Selector Level 3 的规范要求。

### 0.4) 解释器

1. 为方便协程间协作，增加对通道的支持，提供同步读写（阻塞协程）和异步读写方式。
1. 实现 `request` 标签。
1. 在 `archetype` 标签中使用 `type` 指定内容的类型，如 `plain`、 `html`、 `xgml`、 `svg`、`mathml` 等。
1. `in` 属性支持使用 CSS 选择器限定当前文档位置的写法。
1. `init` 标签支持 `as` 可选，从而仅用于初始化一项数据而不绑定变量。
1. `init`、 `bind` 等创建变量的标签支持使用 `_runner` 作为 `at` 属性值，以便创建行者级变量。
1. 检查所有动作元素的实现，确保具有合理的结果数据：
   - 设定 `init` 动作元素的结果数据同绑定的变量数据。
1. `update` 标签。
   - `to` 属性支持 `prepend`、 `remove`、 `insertBefore`、 `insertAfter`、 `intersect`、 `subtract`、 `xor`。
   - `at` 属性支持 `content`。
   - `update` 元素支持同时修改多个数据项，支持 `individually` 副词。
   - `update` 元素可根据模板类型做相应的处理，比如支持命名空间。
1. 完善如下标签从外部数据源获取数据的功能：
   - ~~`init` 标签：支持 `from`、`with` 及 `via` 属性定义的请求参数及方法。~~
   - ~~`archetype` 标签：`src`、`param` 和 `method` 属性的支持。~~
   - `archedata` 标签：`src`、`param` 和 `method` 属性的支持。
   - `error` 标签：`src`、`param` 和 `method` 属性的支持。
   - `except` 标签：`src`、`param` 和 `method` 属性的支持。
   - `define` 标签： 支持 `from`、`with` 及 `via` 属性定义的请求参数及方法。
   - `update` 标签： 支持 `from`、`with` 及 `via` 属性定义的请求参数及方法。
1. 不可捕获错误的产生及处理机制：
   - `error` 标签支持 `type` 属性。

## 1) 当前（202207）

### 1.1) 变体

（无）

### 1.2) 预定义变量

1. 增加、调整或补充预定义变量的实现：
   - ~~`$CRTN`~~
   - ~~完善 `$MATH.eval` 和 `$MATH.eval_l` 对函数及常量的支持（见预定义变量规范）。~~
   - ~~`$SYSTEM` 更名为 `$SYS`~~
   - ~~`$SESSION` 更名为 `$RUNNER`（`sid` 属性更名为 `rid`）~~
   - ~~`$HVML` 更名为 `$CRTN`~~
   - ~~`$REQUEST` 更名为 `$REQ`~~
   - ~~实现 `$EJSON.arith` 和 `$EJSON.bitwise` 方法~~
   - ~~实现 `$STR.nr_bytes` 方法~~
   - 实现 `$CRTN.native_crtn` 方法，返回给定 `cid` 的一个可观察原生实体，随后可用于观察子协程的退出或终止。
1. 评估并合并如下预定义变量的实现：
   - `$FS`
   - `$FILE`

### 1.3) eDOM

1. ~~实现 `void` 目标文档类型。~~

### 1.4) vDOM 解析器

1. ~~对井号注释的初步支持：忽略 HVML 文件中，在 `<DOCTYPE ` 或者 `<!-- ` 开始前，所有以 `#` 打头的行。~~
1. ~~支持在框架元素中定义多个求值表达式或复合求值表达式；参阅 HVML 规范 2.3.4 小节。~~

### 1.5) 解释器

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
   - 将 `$STREAM` 调整为行者级变量。
1. 调整或完善相关实现：
   - 调整调度器实现，通过设置协程进入停止状态以及对应的唤醒条件来实现 ~~协程同步等待数据获取器的返回、等待特定请求的响应消息、主动休眠的超时及提前唤醒、其他协程的退出状态、~~ 并发调用的返回等。
   - ~~`idle` 事件：当所有协程进入事件驱动执行阶段，但未收到任何底层事件的时间累计达到或超过 100ms 时，自动产生 `idle` 事件，广播给所有正在 `$CRTN` 变体上监听 `idle` 事件的协程；`idle` 事件可规约。~~
   - ~~`purc_schedule_vdom()` 支持 `body_id` 参数。~~
   - ~~移除 `sync` 线程。~~
1. 完善如下标签的实现
   - ~~`sleep` 标签：在调度器检查到针对休眠协程的事件时，可由调度器唤醒。~~
   - `call` 和 `return` 标签。并发调用时，克隆 `define` 子树构建新的 vDOM 树。
   - `load` 和 `exit` 标签。
1. 调整求值逻辑：
   - ~~按照规范要求，对 `hvml` 元素内容中定义的表达式进行求值并设置对应栈帧的结果数据；参阅 HVML 规范 2.3.4 小节。~~
   - ~~取消和 `head` 及 `body` （`head` 和 `body` 均为可选框架元素）相关的约束。~~
   - 确保 `hvml` 栈帧始终存在（对应 `_topmost`），在执行观察者代码时，应保留该栈帧存在，该栈帧的结果数据便是整个协程的执行结果。
   - ~~在对动作元素的属性值求值时，若该动作元素定义有表达式内容，则在当前动作元素所在栈帧中完成求值，并将其绑定到当前栈帧的上下文变量 `$0^` 上；若动作元素支持重跑（rerun），则应该在重跑时对内容重新求值并用结果替代 `$0^`；当动作元素需要 `with` 属性值但未定义时，使用内容数据。~~
1. 生成目标文档时，过滤如下特定属性：
   - ~~`hvml` 元素的 `target` 属性。~~
   - ~~外部元素中所有属性名称以 `hvml:` 打头的属性。~~
1. 渲染器对接：
   - ~~HVML 协程对应的页面类型为 `_null` 时，不创建页面，不同步 eDOM 的更新信息。~~
   - ~~增强 PURCMC 协议，用于区分普通文本或标记文本。~~
   - ~~支持页面名称为 `_inherit` 以及 `_self` 的情形。~~

### 1.6) 其他

1. ~~`purc` 命令行工具。~~
1. 代码清理
   - ~~测试程序的许可声明~~
   - API 描述
   - 自定义类型的名称规范化（仅针对结构指针添加 `_t` 后缀）
1. 文档整理。
1. 解决现有测试用例暴露出的缺陷：
   - `test/interpreter/comp/00-fibonacci-void-temp.hvml`：在 `init` 中使用 `#theBody` 作为 `at` 属性值时，解释器报错。
   - `test/interpreter/comp/00-fibonacci-void-temp.hvml`：在动作元素中对 `on`、 `with`、 `onlyif` 和 `while` 属性，使用 `on 2L` 这种指定属性值的语法（不使用等号）时，应按照 EJSON 求值，不转字符串（`on 2L` 的结果应该为 longint 类型 2）。但目前被当做字符串处理。
   - `test/interpreter/test_inherit_document.cpp` 中的 EJSON 字符串生成 VCM 树之后，使用自定义 `$ARGS` 对象替代其中的子字符串，结果不正常。如果删除 `$ARGS.pcid` 之后的空格，会报解析错误。
   - `purc` 命令行：HVML 代码中的 `<li>$< Hello, world! --from COROUTINE-$CRTN.cid</li>` 外部元素的内容，最终添加到 eDOM 之后，被分成了三个文本节点，理论上应该处理为一个文本节点？

## 过往（202206）

### 变体

* ~~优化集合的 `overwrite` 等处理，在已经根据给定的唯一性键键值定位要更新的成员后，逐个更新该成员的其他字段，而不是先移除老的成员再构建一个新成员插入。~~

### eDOM

### 预定义变量

* ~~实现 `$HVML.target` 获取器。~~

### vDOM 解析器

* ~~`body` 标签：一个 HVML 中，支持多个 `body` 标签。~~
* vDOM 构建规则
   1. ~~`match`、`differ` 元素必须作为 `test` 元素的直接子元素。~~

### 解释器

* 检查所有动作标签的实现，确保和规范要求一致：
  1. ~~`except` 标签支持 `type` 属性。~~
  1. ~~`init` 标签支持使用 `via` 属性值 `LOAD` 从外部模块中加载自定义变量：`from` 属性指定外部模块名，`for` 指定模块中的动态对象名。~~
  1. ~~`init` 标签初始化集合时，支持 `casesensitively` 属性和 `caseinsensitively` 属性。~~
  1. ~~`iterate` 标签支持外部类执行器。~~
  1. ~~`observe` 标签支持 `against` 属性。~~
  1. ~~`forget` 标签支持元素汇集。~~
  1. ~~`fire` 标签支持元素汇集。~~
  1. ~~`sort` 标签。~~
  1. ~~`bind` 标签支持 `at` 属性~~
  1. ~~`test` 标签支持 `by` 属性。~~
  1. ~~`hvml` 标签支持 `target` 属性，其他属性原样放入目标文档的根节点。~~
  1. ~~`reduce` 标签~~
  1. ~~`include` 标签~~
  1. ~~`catch` 标签：~~
  1. ~~`back` 标签 `to` 属性的支持~~
  1. ~~`erase` 标签~~
  1. ~~`clear` 标签~~
  1. ~~`choose` 标签~~
  1. ~~`iterate` 标签~~
  1. ~~`inherit` 标签~~
  1. ~~`sleep` 标签：`for` 属性的支持~~
* 延后处理：
  1. `archetype` 标签：`src`、`param` 和 `method` 属性的支持
  1. `archedata` 标签：`src`、`param` 和 `method` 属性的支持
  1. `error` 标签：`src`、`param` 和 `method` 属性的支持
  1. `except` 标签：`src`、`param` 和 `method` 属性的支持
  1. `init` 标签支持使用 `with` 参数定义请求参数，使用 `via` 属性定义请求方法
  1. `define` 标签支持使用 `with` 参数定义请求参数，使用 `via` 属性定义请求方法
  1. `update` 标签：
     - `to` 属性支持 `prepend` 、`remove` 、`insertBefore` 、`insertAfter` 、`insertAfter` 、`intersect` 、`subtract` 、`xor` 、`call`
     - `from` 支持 http 请求支持使用 `with` 参数定义请求参数，使用 `via` 属性定义请求方法
     - `at` 属性支持 `content`。
     - 支持同时修改多个数据项
     - 支持 `individually` 副词
  1. `request` 标签。

## 过往（202205）

### 预定义变量

* 按照[预定义变量规范](https://gitlab.fmsoft.cn/hvml/hvml-docs/-/blob/master/zh/hvml-spec-predefined-variables-v1.0-zh.md)要求调整已有的实现。主要涉及：
   1. ~~`$SYSTEM`~~
   1. ~~`$SESSION`~~
   1. ~~`$HVML`~~
      - `$HVML.target` 方法
   1. ~~`$DATETIME`~~
   1. ~~`$EJSON`~~
   1. `$STREAM`
      - ~~`file://` 的支持~~
* 按照[预定义变量规范](https://gitlab.fmsoft.cn/hvml/hvml-docs/-/blob/master/zh/hvml-spec-predefined-variables-v1.0-zh.md)要求调整或增强预定义变量的实现。主要涉及：
   1. `$URL`
   1. `$FS`
   1. `$FILE`
   1. `$STR`

### 解释器

* 按照[HVML 规范 1.0 RC3](https://gitlab.fmsoft.cn/hvml/hvml-docs/-/blob/master/zh/hvml-spec-v1.0-zh.md#rc3-220501)中的描述调整已有实现，主要有：
   1. ~~`init` 等调整 `at` 属性的使用。~~
   1. `init` 标签 `via` 属性的支持。
   1. ~~`observe` 和 `forget` 支持使用通配符和正则表达式指定待观察或待遗忘的事件名称。~~
   1. ~~`observe` 支持使用 `with` 属性指定已命名的操作组。~~
   1. `observe` 支持针对操作组定义 `$<` 上下文变量。

* 按照[HVML 规范 1.0 RC2](https://gitlab.fmsoft.cn/hvml/hvml-docs/-/blob/master/zh/hvml-spec-v1.0-zh.md#rc2-220401)中的描述调整已有实现，主要有：
   1. ~~局部命名变量可直接使用其名称来引用，相比静态变量，具有较高的名称查找优先级。~~
   1. ~~增强 `iterate` 标签，支持不使用迭代执行器的情形。~~
   1. ~~支持新增的 `against` 介词属性，不再使用 `via` 属性指定初始化集合时的唯一性键名。~~

* ~~调整解析器，根据动作元素中是否包含有 `silently` 副词属性，为 `bool silently` 参数传递相应的实参。~~

* 异常和错误的处理
   1. ~~调整已有的接口或者动态对象方法的实现，静默求值时，对非致命错误（相当于产生可忽略异常；致命错误指内存分配失败等导致程序无法正常运行的错误），应返回一个表示错误状态的有效变体值，如 `false`、`null`、`undefined` 或者空字符串。~~
   1. ~~按规范要求处理异常和错误。~~
   1. ~~`catch` 标签。~~

* 新标签支持
   1. ~~`observe` 支持 `as` 属性命名一个观察者。~~
   1. ~~`fire` 标签。~~
   1. ~~`bind` 标签。~~
   1. ~~`define` 和 `include` 标签。~~
   1. `call` 和 `return` 标签。
   1. `load` 和 `exit` 标签。
   1. `request` 标签。
   1. ~~`differ` 标签；`test` 元素二选一分支的支持（`with` 属性的支持）。~~
   1. ~~`sleep` 标签。~~

* ~~每个解释器实例拥有自己的 RunLoop 事件循环。~~

### 解析器

* ~~不再针对 `via` 属性在 HVML 解析器中做特殊处理，也就是说，`via` 的属性值始终按字符串处理。调整后，只有动作元素的 `on` 和 `with` 属性值才需要特殊处理。~~

### eJSON 解析和求值

* ~~支持 `$#myAnchor?` 这种使用锚定位上下文变量的写法。~~
* ~~支持 CJSONEE（复杂 JSON 表达式）。~~
* ~~eJSON 解析模块，可独立于 HVML 执行栈运行，可支持用户自定义的变量获取接口。~~
* ~~在 eJSON 求值时，增加对可忽略异常的处理~~
   1. ~~调整动态值以及原生实体的获取器及设置器原型，增加 `bool silently` 形参。~~

### 变体

* ~~增加从集合或集合中按照索引值获取成员的接口，以方便代码同时处理数组或集合。~~
* ~~为方便监听变量的状态变化，增加一个新的变体类型：异常（exception）。~~
* ~~调整变体数据结构使用上的一些细节：~~
   1. ~~使用监听器链表头字段的别名 `reserved` 管理保留的变体封装结构，取代当前的循环缓冲区。~~
   1. ~~针对原子字符串和异常，增加一个新的联合字段：`purc_atom_t atom`。~~
   1. ~~仅保留一个监听器链表头结构：将前置监听器放到链表头，后置监听器放到链表尾；遍历时，使用监听器结构中的标志区别监听器类型。~~
   1. ~~无需针对字符串常量及原子字符串统计额外的空间占用。~~
   1. ~~在变体封装结构中增加一个额外字段，在其中为不同的变体类型提供保存额外信息的机制。对字符串类变体，用来保存字符串中的有效 Unicode 字符信息，用于实现 `purc_variant_string_chars()` 接口。~~
* ~~集合一致性的增强：所有作为一致性约束添加到集合中的容器数据，需要做深度复制，将深度克隆后的数据添加到集合中，并在这些新的数据上设置前置监听器，以监听数据变化导致的集合一致性问题。~~

### 数据获取器

* ~~使用网络进程需要检测该进程是否存在（保活、重启）~~
* ~~网络进程缓存、Cookie功能（代码已移植，需要进一步调试验证）~~
* ~~可能的内存泄露？~~

### 工程维护

* ~~不要安装 gtest 以及测试用例到系统中~~。

