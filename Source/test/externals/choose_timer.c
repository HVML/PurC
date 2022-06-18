#include <purc.h>

#include <stdio.h>

extern purc_variant_t
ChooseTimer(purc_variant_t on_value, purc_variant_t with_value)
{
    if (!purc_variant_is_set(on_value)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_string(with_value)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    return purc_variant_set_get_member_by_key_values(on_value, with_value);
}

