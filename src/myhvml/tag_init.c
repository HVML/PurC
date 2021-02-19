/*
** Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
** Copyright (C) 2015-2017 Alexander Borisov
**
** This file is a part of Purring Cat 2, a HVML parser and interpreter.
** 
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
**
** Author: Vincent Wei <https://github.com/VincentWei>
** Author: lex.borisov@gmail.com (Alexander Borisov)
*/

#include "mycore/utils/resources.h"
#include "tag.h"

static const myhvml_tag_context_t myhvml_tag_base_list[MyHVML_TAG_LAST_ENTRY] = 
{
    { MyHVML_TAG__UNDEF, "-undef", 6, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY, 
    },
    { MyHVML_TAG__TEXT, "-text", 5, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY, 
    },
    { MyHVML_TAG__COMMENT, "_comment", 8, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY, 
    },
    { MyHVML_TAG__DOCTYPE, "!DOCTYPE", 8, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY, 
    },
    { MyHVML_TAG__FOREIGN, "_foreign", 8, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY, 
    },
    { MyHVML_TAG_ARCHEDATA, "archedata", 0, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_SPECIAL, 
    },
    { MyHVML_TAG_ARCHETYPE, "archetype", 0, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_SPECIAL | MyHVML_TAG_CATEGORIES_SCOPE, 
    },
    { MyHVML_TAG_BACK, "back", 4, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY, 
    },
    { MyHVML_TAG_BODY, "body", 4, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_SPECIAL, 
    },
    { MyHVML_TAG_CALL, "call", 4, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY,
    },
    { MyHVML_TAG_CATCH, "catch", 5, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY,
    },
    { MyHVML_TAG_CLOSE, "close", 5, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY,
    },
    { MyHVML_TAG_DEFINE, "define", 6, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY,
    },
    { MyHVML_TAG_EMPTY, "empty", 5, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY,
    },
    { MyHVML_TAG_ERROR, "error", 5, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_SPECIAL,
    },
    { MyHVML_TAG_EXCEPT, "except", 6, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_SPECIAL,
    },
    { MyHVML_TAG_HEAD, "head", 4, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_SPECIAL,
    },
    { MyHVML_TAG_HVML, "hvml", 4, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_SPECIAL,
    },
    { MyHVML_TAG_INIT, "init", 4, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_SPECIAL,
    },
    { MyHVML_TAG_INCLUDE, "include", 7, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_SPECIAL,
    },
    { MyHVML_TAG_ITERATE, "iterate", 7, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY,
    },
    { MyHVML_TAG_LISTEN, "listen", 6, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY,
    },
    { MyHVML_TAG_LOAD, "load", 4, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY,
    },
    { MyHVML_TAG_MATCH, "match", 5, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY,
    },
    { MyHVML_TAG_OBSERVE, "observe", 7, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY,
    },
    { MyHVML_TAG_RAWTEXT, "rawtext", 7, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_SPECIAL,
    },
    { MyHVML_TAG_REMOVE, "remove", 6, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY,
    },
    { MyHVML_TAG_REQUEST, "request", 7, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY,
    },
    { MyHVML_TAG_REDUCE, "reduce", 6, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY,
    },
    { MyHVML_TAG_RETURN, "return", 6, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY,
    },
    { MyHVML_TAG_SET, "set", 3, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY,
    },
    { MyHVML_TAG_TEST, "test", 4, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY,
    },
    { MyHVML_TAG_UPDATE, "update", 6, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY,
    },
    {MyHVML_TAG__END_OF_FILE, "-end-of-file", 12, MyHVML_TOKENIZER_STATE_DATA, 
            MyHVML_TAG_CATEGORIES_ORDINARY,
    }
};

#include "tag_static_list.inc"

#if SIZEOF_PTR == 8
// 2^40 + 2^8 + 0xb3 = 1099511628211
#   define FNV_PRIME        ((size_t)0x100000001b3ULL)
#   define FNV_INIT         ((size_t)0xcbf29ce484222325ULL)
#else
// 2^24 + 2^8 + 0x93 = 16777619
#   define FNV_PRIME        ((size_t)0x01000193)
#   define FNV_INIT         ((size_t)0x811c9dc5)
#endif

static size_t str2key (const char* str, int length)
{
    const unsigned char* s = (const unsigned char*)str;
    size_t hval = FNV_INIT;

    if (str == NULL || length <= 0)
        return 0;

    /*
     * FNV-1a hash each octet in the buffer
     */
    while (*s && length) {

        /* xor the bottom with the current octet */
        hval ^= (size_t)*s++;
        length--;

        /* multiply by the FNV magic prime */
#ifdef __GNUC__
#   if SIZEOF_PTR == 8
        hval += (hval << 1) + (hval << 4) + (hval << 5) +
            (hval << 7) + (hval << 8) + (hval << 40);
#   else
        hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
#   endif
#else
        hval *= FNV_PRIME;
#endif
    }

    /* return our new hash value */
    return hval;
}

const myhvml_tag_context_t * myhvml_tag_static_search(const char* name, size_t length)
{
    size_t idx;

    if (mycore_strncmp (name, "v:", 2) == 0) {
        name += 2;
        length -= 2;
    }

    idx = str2key (name, length);
    if (idx == 0)
        return NULL;

    idx %= MyHVML_BASE_STATIC_SIZE;
    idx += 1;

    while (myhvml_tag_static_list_index[idx].ctx) {
        if (myhvml_tag_static_list_index[idx].ctx->name_length == length) {
            if (mycore_strncmp(myhvml_tag_static_list_index[idx].ctx->name, name, length) == 0)
                return myhvml_tag_static_list_index[idx].ctx;

            if (myhvml_tag_static_list_index[idx].next)
                idx = myhvml_tag_static_list_index[idx].next;
            else
                return NULL;
        }
        else if (myhvml_tag_static_list_index[idx].ctx->name_length > length) {
            return NULL;
        }
        else {
            idx = myhvml_tag_static_list_index[idx].next;
        }
    }

    return NULL;
}

const myhvml_tag_context_t * myhvml_tag_static_get_by_id(size_t idx)
{
    return &myhvml_tag_base_list[idx];
}

