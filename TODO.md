# TODO 清单

## 2022-02-12

### 增强和调整变体的实现

1. 为方便监听变量的状态变化，增加一个新的变体类型：异常（exception）。
1. 调整变体数据结构使用上的一些细节：
   - 针对原子字符串和异常，可考虑增加一个新的联合字段：`purc_atom_t atom`。
   - 仅保留一个监听器链表头结构：将前置监听器放到链表头，后置监听器放到链表尾；遍历时，使用监听器结构中的标志区别监听器类型。
   - 无需针对字符串常量及原子字符串统计额外的空间占用。
1. 字符串字符数量信息的维护，可考虑三种方案：
   - 在 `sz_ptr[0]` 中保存字符串的字符数量而非字节数量？一方面在构造字符串变体时，已经解析过一次编码了，字符数量信息已经存在；另一方面，毕竟解码 UTF-8 计算字符个数要比单纯调用 `strlen()` 耗时。
   - 在不可变变体上监听其引用计数的变化是否有价值？如果没有，则可利用监听器链表头字段做一个联合，在其中为不可变变体保存一些额外的信息，比如对字符串，保留其字符个数。
   - 使用一个额外的数据结构来代表一个字符串，而不仅仅保存字符串的指针。在该数据结构中，可额外保存字符长度等信息。另外，在将来支持 Unicode 相关处理时，可对这个数据结构做进一步扩充，比如保存 Unicode 码点，逻辑序列转换为视觉序列等。

### 可忽略异常的处理

明确定义哪些异常是可忽略的（ignorable）。对可忽略异常，当在动态值或者原生实体的方法中传入可忽略标志时，不产生异常而返回标志错误（如 `false`）的返回值。

1. 调整动态值以及原生实体的获取器及设置器原型，增加 `bool silently` 形参。
1. 调整解析器，根据动作元素中是否包含有 `silentyl` 副词属性，为 `bool silently` 参数传递相应的实参。

当前定义的可忽略异常有：

1. `BadEncoding`
1. `NoSuchKey`
1. `DuplicateKey`
1. `AccessDenied`
1. `TooLong`
1. `TooMany`
1. `EntityNotFound`

### 增强集合的实现

1. 所有作为一致性约束添加到集合中的容器数据，需要做深度复制，将深度克隆后的数据添加到集合中，并在这些新的数据上设置前置监听器，以监听数据变化导致的集合一致性问题。

### 按照规范要求调整或增强预定义变量的实现

<https://gitlab.fmsoft.cn/hvml/hvml-docs/-/blob/master/zh/hvml-spec-predefined-variables-v1.0-zh.md>

主要涉及：

1. `$STR`
1. `$URL`
1. `$FS`

### 跟踪 eDOM 的变化

1. 跟踪 eDOM 的变化，使之可以动态更新已有的元素汇集。

### 完整的 CSS 选择器实现

1. 按照 CSS Selector Level 3 的规范要求实现选择器。

## 2021-12-10

For `set` variant, it's more or less like a relational-table with primary keys.
As result, `set` shall remain as a valid one when add/update/del operation
occurs.
```
    set: {!"id", {id:1, name:'foo'}, {id:2, name:'bar'}}
    when update_by_key(set, 1, 2), it shall report failure as update-conflict
```

## 2021-12-15

need to be optimized
```
struct pcintr_element_ops*
pcintr_get_element_ops(pcvdom_element_t element)
{
    PC_ASSERT(element);

    switch (element->tag_id) {
        case PCHVML_TAG_ITERATE:
            return pcintr_iterate_get_ops();
        default:
            PC_ASSERT(0); // Not implemented yet
            return NULL;
    }
}

```

## 2020-12-18

concerning about variant loaded from external dynamic library:

1. only object-variant can be loaded, which results in both dev-friendly problem
   and running-performance
2. isolation problem: internally-hidden-field in object to store the handle.
3. performance problem: when refcount reaches 0, bunch of steps shall be take
   to check if it's loaded-variant
4. currently, `purc_variant_unload_dvobj` was introduced to tackle such issue,
   which tastes bad and failed with life-time conflicts reported by `valgrind`

we might solve this problem by introducing an internal type of variant, called
PHANTOM...

