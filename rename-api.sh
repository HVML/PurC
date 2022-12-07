#!/bin/bash

# NOTE: run this script in `Source/` directory.

# Names for object variant:
# sed -i 's/\<purc_variant_compare_method\>/pcvrnt_compare_method/g' `grep '\<pcvariant_compare_method\>' * -rl`
# sed -i 's/\<purc_variant_object_iterator\>/pcvrnt_object_iterator/g' `grep '\<purc_variant_object_iterator\>' * -rl`
# sed -i 's/purc_variant_object_iterator_create_begin/pcvrnt_object_iterator_create_begin/g' `grep purc_variant_object_iterator_create_begin * -rl`
# sed -i 's/purc_variant_object_iterator_create_end/pcvrnt_object_iterator_create_end/g' `grep purc_variant_object_iterator_create_end * -rl`
# sed -i 's/purc_variant_object_iterator_release/pcvrnt_object_iterator_release/g' `grep purc_variant_object_iterator_release * -rl`
# sed -i 's/purc_variant_object_iterator_next/pcvrnt_object_iterator_next/g' `grep purc_variant_object_iterator_next * -rl`
# sed -i 's/purc_variant_object_iterator_prev/pcvrnt_object_iterator_prev/g' `grep purc_variant_object_iterator_prev * -rl`
# sed -i 's/purc_variant_object_iterator_get_key/pcvrnt_object_iterator_get_key/g' `grep purc_variant_object_iterator_get_key * -rl`
# sed -i 's/purc_variant_object_iterator_get_ckey/pcvrnt_object_iterator_get_ckey/g' `grep purc_variant_object_iterator_get_ckey * -rl`
# sed -i 's/purc_variant_object_iterator_get_value/pcvrnt_object_iterator_get_value/g' `grep purc_variant_object_iterator_get_value * -rl`

#sed -i 's/purc_variant_ejson_parse_tree/purc_ejson_parsing_tree/g' `grep purc_variant_ejson_parse_tree * -rl`
#sed -i 's/purc_ejson_parse_tree/purc_ejson_parsing_tree/g' `grep purc_ejson_parse_tree * -rl`

#sed -i 's/\<pcvrnt_compare_method\>/pcvrnt_compare_cb/g' `grep '\<pcvrnt_compare_method\>' * -rl`
#sed -i 's/\<purc_variant_compare_opt\>/pcvrnt_compare_method/g' `grep '\<purc_variant_compare_opt\>' * -rl`
#sed -i 's/\<purc_vrtcmp_opt_t\>/pcvrnt_compare_method_k/g' `grep '\<purc_vrtcmp_opt_t\>' * -rl`
#sed -i 's/PCVARIANT_COMPARE_OPT_/PCVRNT_COMPARE_METHOD_/g' `grep PCVARIANT_COMPARE_OPT_ * -rl`

# sed -i 's/FFFF/TTTT/g' `grep FFFF * -rl`
# sed -i 's/\<FFFF\>/TTTT/g' `grep '\<FFFF\>' * -rl`

sed -i 's/PCVARIANT_/PCVRNT_/g' `grep PCVARIANT_ * -rl`
