# TODO List

## 长期

### 变体

* 实现新的线性容器类型元组（tuple）。

### eDOM

* 优化元素汇集原生实体的实现，使之可以处理 eDOM 变化的情形，并符合 CSS Selector Level 3 的规范要求。

### eJSON 解析和求值

* 支持元组。

### 解释器

* 不可捕获错误的产生及处理机制：
  1. `error` 标签支持 `type` 属性。

## 202207

### 变体

* 在预定义变量的实现中，使用线性容器封装接口获取容器类参数的大小及其成员。

### 预定义变量

* 增加、调整或补充预定义变量的实现：
  1. `$RDR`
  1. `$URL`
  1. `$FS`
  1. `$FILE`
  1. `$STR`

### eDOM

* 实现 `void` 目标文档类型。

### 解释器

* 支持 `request` 标签。
* 支持多个 `body` 标签。
* `call`、`load` 标签支持创建新行者。
* `exit` 标签支持 `with` 属性。
* 完善如下标签从外部数据源获取数据的功能：
  1. ~~`init` 标签：支持 `from`、`with` 及 `via` 属性定义的请求参数及方法。~~
  1. ~~`archetype` 标签：`src`、`param` 和 `method` 属性的支持。~~
  1. `archedata` 标签：`src`、`param` 和 `method` 属性的支持。
  1. `error` 标签：`src`、`param` 和 `method` 属性的支持。
  1. `except` 标签：`src`、`param` 和 `method` 属性的支持。
  1. `define` 标签： 支持 `from`、`with` 及 `via` 属性定义的请求参数及方法。
  1. `update` 标签： 支持 `from`、`with` 及 `via` 属性定义的请求参数及方法。
* `update` 标签。
  1. `to` 属性支持 `prepend` 、`remove` 、`insertBefore` 、`insertAfter` 、`insertAfter` 、`intersect` 、`subtract` 、`xor`。
  1. `at` 属性支持 `content`。
  1.  支持同时修改多个数据项，支持 `individually` 副词。
* `sleep` 标签在调度器检查到有针对休眠协程的事件时，可由调度器唤醒。

## 202206

### 变体

* 容器子孙成员变化后在容器变体上产生 `change` 事件。
* ~~优化集合的 `overwrite` 等处理，在已经根据给定的唯一性键键值定位要更新的成员后，逐个更新该成员的其他字段，而不是先移除老的成员再构建一个新成员插入。~~

### eDOM

### 预定义变量

* ~~实现 `$HVML.target` 获取器。~~
* 实现文档级 `$REQUEST` 预定义变量，将该变量和 `purc_schedule_vdom` 函数中的 `request` 参数关联。

### vDOM 解析器

* ~~`body` 标签：一个 HVML 中，支持多个 `body` 标签。~~
* vDOM 构建规则
   1. ~~`match`、`differ` 元素必须作为 `test` 元素的直接子元素。~~

### 解释器

* 检查所有动作标签的实现，确保和规范要求一致：
  1. ~~`except` 标签支持 `type` 属性。~~
  1. ~~`init` 标签支持使用 `via` 属性值 `LOAD` 从外部模块中加载自定义变量：`from` 属性指定外部模块名，`for` 指定模块中的动态对象名。~~
  1. ~~`init` 标签初始化集合时，支持 `casesensitively` 属性和 `caseinsensitively` 属性。~~
  1. `call` 标签。
  1. `return` 标签。
  1. `load` 标签。
  1. `exit` 标签（不含对 `with` 属性的支持）。
  1. `iterate` 标签支持外部类执行器。
  1. `observe` 标签支持上下文变量: `$!` 和 `$@`。
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

## 202205

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

