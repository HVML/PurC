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

