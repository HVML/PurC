# TODO 清单

## 2022-02-12

### 增强和调整变体的实现

1. 为方便监听变量的状态变化，增加一个新的变体类型：异常（exception）。
1. 调整变体数据结构使用上的一些细节：
   - 字符串原子和异常类型，不需要统计其空间占用。可考虑增加一个新的联合字段：`purc_atom_t atom`。
   - 仅保留单个监听器链表头结构：将前置监听器放到链表头，后置监听器放到链表尾；遍历时，使用监听器结构中的标志区别监听器类型。
   - 静态字符串是否需要统计其空间占用？
   - 保存字符串的字符数量而非字节数量，是否更有意义？毕竟 UTF-8 的解码要比单纯调用 `strlen()` 耗时。

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

