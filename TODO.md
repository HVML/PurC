# TODO list for PurC

## [2020.12.10]

For `set` variant, it's more or less like a relational-table with primary keys.
As result, `set` shall remain as a valid one when add/update/del operation
occurs.
```
    set: {!"id", {id:1, name:'foo'}, {id:2, name:'bar'}}
    when update_by_key(set, 1, 2), it shall report failure as update-conflict
```

