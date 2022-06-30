#include <purc.h>

extern purc_variant_t
get_member(purc_variant_t on_value, purc_variant_t with_value)
{
    if (!purc_variant_is_object(on_value)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_string(with_value)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v = purc_variant_object_get(on_value, with_value);
    if (v == PURC_VARIANT_INVALID)
        v = purc_variant_make_undefined();
    else
        purc_variant_ref(v);

    return v;
}

extern purc_variant_t
to_array(purc_variant_t on_value, purc_variant_t with_value)
{
    purc_variant_t v = purc_variant_make_array(2, on_value, with_value);
    if (v == PURC_VARIANT_INVALID)
        v = purc_variant_make_undefined();

    return v;
}

extern purc_variant_t
statsUserRegion(purc_variant_t on_value, purc_variant_t with_value)
{
    if (!on_value || !with_value
            || !purc_variant_is_array(on_value)
            || !purc_variant_is_string(with_value)) {
        return purc_variant_make_undefined();
    }

    purc_variant_t result = purc_variant_make_object(0, PURC_VARIANT_INVALID,
            PURC_VARIANT_INVALID);
    if (result == PURC_VARIANT_INVALID) {
        return purc_variant_make_undefined();
    }

    purc_variant_t regions = purc_variant_make_object(0, PURC_VARIANT_INVALID,
            PURC_VARIANT_INVALID);
    if (regions == PURC_VARIANT_INVALID) {
        purc_variant_unref(result);
        return purc_variant_make_undefined();
    }

    size_t nr_size = purc_variant_array_get_size(on_value);

    purc_variant_t count = purc_variant_make_ulongint(nr_size);
    purc_variant_t count_key = purc_variant_make_string("count", false);
    purc_variant_object_set(result, count_key, count);
    purc_variant_unref(count_key);
    purc_variant_unref(count);

    purc_variant_t regions_key = purc_variant_make_string("regions", false);
    purc_variant_object_set(result, regions_key, regions);
    purc_variant_unref(regions_key);
    purc_variant_unref(regions);

    purc_variant_t unknown_key = purc_variant_make_string("unknown", false);

    for (size_t i = 0; i < nr_size; i++) {
        purc_variant_t member = purc_variant_array_get(on_value, i);
        if (!purc_variant_is_object(member)) {
            continue;
        }

        purc_variant_t mv = purc_variant_object_get(member, with_value);
        if (mv) {
            purc_variant_t counter = purc_variant_object_get(regions, mv);
            uint64_t count = 0;
            if (counter) {
                purc_variant_cast_to_ulongint(counter, &count, false);
            }
            count += 1;
            purc_variant_t ncounter = purc_variant_make_ulongint(count);
            if (!count) {
                break;
            }
            purc_variant_object_set(regions, mv, ncounter);
            purc_variant_unref(ncounter);
        }
        else {
            purc_variant_t counter = purc_variant_object_get(regions,
                    unknown_key);
            uint64_t count = 0;
            if (counter) {
                purc_variant_cast_to_ulongint(counter, &count, false);
            }
            count += 1;
            purc_variant_t ncounter = purc_variant_make_ulongint(count);
            if (!count) {
                break;
            }
            purc_variant_object_set(regions, unknown_key, ncounter);
            purc_variant_unref(ncounter);
        }
    }
    purc_variant_unref(unknown_key);
    return result;
}

extern purc_variant_t
to_sort(purc_variant_t on_value, purc_variant_t with_value,
        purc_variant_t against_value, bool desc, bool caseless)
{
    // TODO: check arguments and pay attention to the whole outcome
    // after dlclose

    // if (on_value == PURC_VARIANT_INVALID ||
    //         purc_variant_is_array(on_value))
    // {
    //     purc_set_error(PURC_ERROR_INVALID_VALUE);
    //     return PURC_VARIANT_INVALID;
    // }

    // if (with_value == PURC_VARIANT_INVALID ||
    //         purc_variant_is_array(with_value))
    // {
    //     purc_set_error(PURC_ERROR_INVALID_VALUE);
    //     return PURC_VARIANT_INVALID;
    // }

    // if (against_value == PURC_VARIANT_INVALID ||
    //         purc_variant_is_array(against_value))
    // {
    //     purc_set_error(PURC_ERROR_INVALID_VALUE);
    //     return PURC_VARIANT_INVALID;
    // }

    // this is just to demonstrate how to use external sorter
    // so we just intentionally return an array containing
    // on_value/with_value/against_value/desc/caseless
    // all together

    if (with_value == PURC_VARIANT_INVALID) {
        with_value = purc_variant_make_undefined();
    }
    else {
        with_value = purc_variant_ref(with_value);
    }

    if (against_value == PURC_VARIANT_INVALID) {
        against_value = purc_variant_make_undefined();
    }
    else {
        against_value = purc_variant_ref(against_value);
    }

    char buf1[16], buf2[16];
    snprintf(buf1, sizeof(buf1), "%s", desc ? "desc" : "asc");
    snprintf(buf2, sizeof(buf2), "%s", caseless ? "caseless" : "casesensitive");

    purc_variant_t d = purc_variant_make_string(buf1, false);
    purc_variant_t c = purc_variant_make_string(buf2, false);

    purc_variant_t v = PURC_VARIANT_INVALID;

    if (d != PURC_VARIANT_INVALID && c != PURC_VARIANT_INVALID) {
        v = purc_variant_make_array(5, on_value, with_value, against_value,
                d, c);
    }

    PURC_VARIANT_SAFE_CLEAR(c);
    PURC_VARIANT_SAFE_CLEAR(d);
    PURC_VARIANT_SAFE_CLEAR(against_value);
    PURC_VARIANT_SAFE_CLEAR(with_value);
    return v;
}

