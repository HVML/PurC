/**
 * @file encoding.c
 * @author
 * @date 2021/07/02
 * @brief The complementation of encoding in parsing html.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */



#include "html/html/encoding.h"

#include "html/core/str.h"


static const unsigned char *
pchtml_html_encoding_meta(pchtml_html_encoding_t *em,
                       const unsigned char *data, const unsigned char *end);

static const unsigned char *
pchtml_html_get_attribute(const unsigned char *data, const unsigned char *end,
                       const unsigned char **name, const unsigned char **name_end,
                       const unsigned char **value, const unsigned char **value_end);


static inline const unsigned char *
pchtml_html_encoding_skip_spaces(const unsigned char *data, const unsigned char *end)
{
    for (; data < end; data++) {
        switch (*data) {
            case 0x09: case 0x0A:
            case 0x0C: case 0x0D:
            case 0x20:
                break;

            default:
                return data;
        }
    }

    return end;
}

static inline const unsigned char *
pchtml_html_encoding_skip_name(const unsigned char *data, const unsigned char *end)
{
    for (; data < end; data++) {
        switch (*data) {
            case 0x09: case 0x0A:
            case 0x0C: case 0x0D:
            case 0x20: case 0x3E:
                return data;
        }
    }

    return end;
}

static inline const unsigned char *
pchtml_html_encoding_tag_end(const unsigned char *data, const unsigned char *end)
{
    data = memchr(data, '>', (end - data));
    if (data == NULL) {
        return end;
    }

    return data + 1;
}

unsigned int
pchtml_html_encoding_init(pchtml_html_encoding_t *em)
{
    unsigned int status;

    if (em == NULL) {
        return PCHTML_STATUS_ERROR_WRONG_ARGS;
    }

    status = pchtml_array_obj_init(&em->cache, 12,
                                   sizeof(pchtml_html_encoding_entry_t));
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    return pchtml_array_obj_init(&em->result, 12,
                                 sizeof(pchtml_html_encoding_entry_t));
}

pchtml_html_encoding_t *
pchtml_html_encoding_destroy(pchtml_html_encoding_t *em, bool self_destroy)
{
    if (em == NULL) {
        return NULL;
    }

    pchtml_array_obj_destroy(&em->cache, false);
    pchtml_array_obj_destroy(&em->result, false);

    if (self_destroy) {
        return pchtml_free(em);
    }

    return em;
}

unsigned int
pchtml_html_encoding_determine(pchtml_html_encoding_t *em,
                            const unsigned char *data, const unsigned char *end)
{
    const unsigned char *name, *name_end;
    const unsigned char *value, *value_end;

    while (data < end) {
        /* Find tag beginning */
        data = memchr(data, '<', (end - data));
        if (data == NULL) {
            return PCHTML_STATUS_OK;
        }

        if (++data == end) {
            return PCHTML_STATUS_OK;
        }

        switch (*data) {
            /* Comment or broken tag */
            case '!':
                if ((data + 5) > end) {
                    return PCHTML_STATUS_OK;
                }

                if (data[1] != '-' || data[2] != '-') {
                    data = pchtml_html_encoding_tag_end(data, end);
                    continue;
                }

                while (data < end) {
                    data = pchtml_html_encoding_tag_end(data, end);

                    if (data[-3] == '-' && data[-2] == '-') {
                        break;
                    }
                }

                break;

            case '?':
                data = pchtml_html_encoding_tag_end(data, end);
                break;

            case '/':
                data++;

                if ((data + 3) > end) {
                    return PCHTML_STATUS_OK;
                }

                if ((unsigned) (*data - 0x41) <= (0x5A - 0x41)
                    || (unsigned) (*data - 0x61) <= (0x7A - 0x61))
                {
                    goto skip_attributes;
                }

                data = pchtml_html_encoding_tag_end(data, end);
                break;

            default:

                if ((unsigned) (*data - 0x41) > (0x5A - 0x41)
                    && (unsigned) (*data - 0x61) > (0x7A - 0x61))
                {
                    break;
                }

                if ((data + 6) > end) {
                    return PCHTML_STATUS_OK;
                }

                if (!pchtml_str_data_ncasecmp(data, (unsigned char *) "meta", 4)) {
                    goto skip_attributes;
                }

                data += 4;

                switch (*data++) {
                    case 0x09: case 0x0A: case 0x0C:
                    case 0x0D: case 0x20: case 0x2F:
                        break;

                    default:
                        goto skip_attributes;
                }

                data = pchtml_html_encoding_meta(em, data, end);
                if (data == NULL) {
                    return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
                }

                break;

            skip_attributes:

                data = pchtml_html_encoding_skip_name(data, end);
                if (data >= end) {
                    return PCHTML_STATUS_OK;
                }

                if (*data == '>') {
                    data++;
                    continue;
                }

                /* Skip attributes */
                while (data < end) {
                    data = pchtml_html_get_attribute(data, end, &name, &name_end,
                                                  &value, &value_end);
                    if (name == NULL) {
                        break;
                    }
                }

                break;
        }
    }

    return PCHTML_STATUS_OK;
}

static const unsigned char *
pchtml_html_encoding_meta(pchtml_html_encoding_t *em,
                       const unsigned char *data, const unsigned char *end)
{
    size_t i, len, cur;
    bool got_pragma, have_content;
    uint8_t need_pragma;
    const unsigned char *name, *name_end;
    const unsigned char *value, *value_end;
    pchtml_html_encoding_entry_t *attr;

    got_pragma = false;
    have_content = false;
    need_pragma = 0x00;
    cur = pchtml_array_obj_length(&em->result);

    pchtml_array_obj_clean(&em->cache);

    while (data < end) {

    find_attr:

        data = pchtml_html_get_attribute(data, end, &name, &name_end,
                                      &value, &value_end);
        if (name == NULL) {
            break;
        }

        len = name_end - name;

        if (len < 7) {
            continue;
        }

        /* Exists check */
        for (i = 0; i < pchtml_array_obj_length(&em->cache); i++) {
            attr = pchtml_array_obj_get(&em->cache, i);

            if ((attr->end - attr->name) == (int)len
                && pchtml_str_data_ncasecmp(attr->name, name, len))
            {
                goto find_attr;
            }
        }

        /* Append attribute to cache */
        attr = pchtml_array_obj_push(&em->cache);
        if (attr == NULL) {
            return NULL;
        }

        attr->name = name;
        attr->end = name_end;

        if (value == NULL) {
            continue;
        }

        /* http-equiv check */
        if (len == (sizeof("http-equiv") - 1)) {
            if (!pchtml_str_data_ncasecmp((unsigned char *) "http-equiv", name, len)) {
                continue;
            }

            if ((value_end - value) == (sizeof("content-type") - 1)
                && pchtml_str_data_ncasecmp((unsigned char *) "content-type",
                                            value, (sizeof("content-type") - 1)))
            {
                got_pragma = true;
            }

            continue;
        }

        if (pchtml_str_data_ncasecmp((unsigned char *) "content", name, 7)) {
            if (have_content == false) {

                name = pchtml_html_encoding_content(value, value_end, &name_end);
                if (name == NULL) {
                    continue;
                }

                attr = pchtml_array_obj_push(&em->result);
                if (attr == NULL) {
                    return NULL;
                }

                attr->name = name;
                attr->end = name_end;

                need_pragma = 0x02;
                have_content = true;
            }

            continue;
        }

        if (pchtml_str_data_ncasecmp((unsigned char *) "charset", name, 7)) {
            attr = pchtml_array_obj_push(&em->result);
            if (attr == NULL) {
                return NULL;
            }

            attr->name = value;
            attr->end = value_end;

            need_pragma = 0x01;
        }
    }

    if (need_pragma == 0x00 || (need_pragma == 0x02 && got_pragma == false)) {
        if (cur != pchtml_array_obj_length(&em->result)) {
            pchtml_array_obj_pop(&em->result);
        }
    }

    return data;
}

const unsigned char *
pchtml_html_encoding_content(const unsigned char *data, const unsigned char *end,
                          const unsigned char **name_end)
{
    const unsigned char *name;

    do {
        for (; (data + 7) < end; data++) {
            if (pchtml_str_data_ncasecmp((unsigned char *) "charset", data, 7)) {
                goto found;
            }
        }

        return NULL;

    found:

        data = pchtml_html_encoding_skip_spaces((data + 7), end);
        if (data >= end) {
            return NULL;
        }

        if (*data != '=') {
            continue;
        }

        data = pchtml_html_encoding_skip_spaces((data + 1), end);
        if (data >= end) {
            return NULL;
        }

        break;
    }
    while (true);

    if (*data == '\'' || *data == '"') {
        *name_end = data++;
        name = data;

        for (; data < end; data++) {
            if (*data == **name_end) {
                break;
            }
        }

        *name_end = data;
        goto done;
    }

    name = data;
    *name_end = data;

    for (; data < end; data++) {
        switch (*data) {
            case ';':
                goto done;

            case 0x09: case 0x0A:
            case 0x0C: case 0x0D:
            case 0x20:
                goto done;

            case '"':
            case '\'':
                return NULL;
        }
    }

    if (data == name) {
        return NULL;
    }

done:

    *name_end = data;

    return name;
}

static const unsigned char *
pchtml_html_get_attribute(const unsigned char *data, const unsigned char *end,
                       const unsigned char **name, const unsigned char **name_end,
                       const unsigned char **value, const unsigned char **value_end)
{
    unsigned char ch;

    *name = NULL;
    *value = NULL;

    for (; data < end; data++) {
        switch (*data) {
            case 0x09: case 0x0A:
            case 0x0C: case 0x0D:
            case 0x20: case 0x2F:
                break;

            case 0x3E:
                return (data + 1);

            default:
                goto name_state;
        }
    }

    if (data == end) {
        return data;
    }

name_state:

    /* Attribute name */
    *name = data;

    while (data < end) {
        switch (*data) {
            case 0x09: case 0x0A:
            case 0x0C: case 0x0D:
            case 0x20:
                *name_end = data;

                data++;
                goto spaces_state;

            case '/': case '>':
                *name_end = data;
                return data;

            case '=':
                if (*name != NULL) {
                    *name_end = data++;
                    goto value_state;
                }
        }

        data++;
    }

spaces_state:

    data = pchtml_html_encoding_skip_spaces(data, end);
    if (data == end) {
        return data;
    }

    if (*data != '=') {
        return data;
    }

value_state:

    data = pchtml_html_encoding_skip_spaces(data, end);
    if (data == end) {
        return data;
    }

    switch (*data) {
        case '"':
        case '\'':
            ch = *data++;
            if (data == end) {
                return data;
            }

            *value = data;

            do {
                if (*data == ch) {
                    *value_end = data;
                    return data + 1;
                }
            }
            while (++data < end);

            *value = NULL;

            return data;

        case '>':
            return data;

        default:
            *value = data++;
            break;
    }

    for (; data < end; data++) {
        switch (*data) {
            case 0x09: case 0x0A:
            case 0x0C: case 0x0D:
            case 0x20: case 0x3E:
                *value_end = data;
                return data;
        }
    }

    *value = NULL;

    return data;
}

/*
 * No inline functions for ABI.
 */
pchtml_html_encoding_t *
pchtml_html_encoding_create_noi(void)
{
    return pchtml_html_encoding_create();
}

void
pchtml_html_encoding_clean_noi(pchtml_html_encoding_t *em)
{
    pchtml_html_encoding_clean(em);
}

pchtml_html_encoding_entry_t *
pchtml_html_encoding_meta_entry_noi(pchtml_html_encoding_t *em, size_t idx)
{
    return pchtml_html_encoding_meta_entry(em, idx);
}

size_t
pchtml_html_encoding_meta_length_noi(pchtml_html_encoding_t *em)
{
    return pchtml_html_encoding_meta_length(em);
}

pchtml_array_obj_t *
pchtml_html_encoding_meta_result_noi(pchtml_html_encoding_t *em)
{
    return pchtml_html_encoding_meta_result(em);
}
