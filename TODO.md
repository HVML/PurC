# TODO list for PurC

## [2020.12.10]

For `set` variant, it's more or less like a relational-table with primary keys.
As result, `set` shall remain as a valid one when add/update/del operation
occurs.
```
    set: {!"id", {id:1, name:'foo'}, {id:2, name:'bar'}}
    when update_by_key(set, 1, 2), it shall report failure as update-conflict
```

## [2020.12.15]
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

## [2020.12.18]
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

