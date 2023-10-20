/*
 * @file app-manifest.c
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2023/10/20
 * @brief The app manifest of PurC.
 *
 * Copyright (C) 2023 FMSoft <https://www.fmsoft.cn>
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

#define _GNU_SOURCE
#include "config.h"

#include "purc.h"

#include "private/instance.h"
#include "private/debug.h"

#define FALLBACK_LOCALE     "en_US"
#define FALLBACK_DENSITY    "hdpi"

#define KEY_LABEL           "label"
#define KEY_DESC            "description"
#define KEY_ICON            "icon"

static const char *label_for_unlabeled_app =
    "{"
        "en: 'Unlabeled',"
        "zh_CN: '未标记',"
        "zh: '未標記'"
    "}";

static const char *desc_for_unlabeled_app =
    "{"
        "en: 'An unlabeled HVML app.',"
        "zh_CN: '尚未标记的 HVML 应用。',"
        "zh: '尚未標記的 HVML 應用。'"
    "}";

static const char *icon_for_unlabeled_app =
    "{"
        "hdpi: null"
    "}";

purc_variant_t
pcinst_load_app_manifest(const char *app_name)
{
    char path_buf[(sizeof(PURC_PATH_APP_MANIFEST) + PURC_LEN_APP_NAME)];

    int n = snprintf(path_buf, sizeof(path_buf), PURC_PATH_APP_MANIFEST,
            app_name);
    if (n < 0 || (size_t)n >= sizeof(path_buf)) {
        PC_ERROR("Failed to make path to the manifest file for app %s\n",
                app_name);
        goto failed;
    }

    purc_variant_t manifest;
    manifest = purc_variant_load_from_json_file(path_buf);
    printf("manifest path: %s (%p)\n", path_buf, manifest);

    if (manifest == PURC_VARIANT_INVALID) {
        printf("Failed to load manifest for app %s: %s\n",
                app_name,
                purc_get_error_message(purc_get_last_error()));
        purc_clr_error();
        manifest = purc_variant_make_object_0();
    }
    else if (!purc_variant_is_object(manifest)) {
        printf("A bad manifest file for app %s\n", app_name);
        purc_variant_unref(manifest);
        manifest = purc_variant_make_object_0();
    }

    /* make sure that manifest contains the required properties */
    purc_variant_t v, fallback;
    v = purc_variant_object_get_by_ckey(manifest, KEY_LABEL);
    if (v == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string(v) && !purc_variant_is_object(v))) {
        fallback = purc_variant_make_from_json_string(label_for_unlabeled_app,
                sizeof(label_for_unlabeled_app) - 1);
        if (fallback == PURC_VARIANT_INVALID) {
            goto failed;
        }
        purc_variant_object_set_by_ckey(manifest, KEY_LABEL, fallback);
        purc_variant_unref(fallback);
    }
    else if (purc_variant_is_object(v)) {
        fallback = purc_variant_make_from_json_string(label_for_unlabeled_app,
                sizeof(label_for_unlabeled_app) - 1);
        if (fallback == PURC_VARIANT_INVALID) {
            goto failed;
        }
        purc_variant_object_unite(v, fallback, PCVRNT_CR_METHOD_IGNORE);
        purc_variant_unref(fallback);
    }

    v = purc_variant_object_get_by_ckey(manifest, KEY_DESC);
    if (v == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string(v) && !purc_variant_is_object(v))) {
        fallback = purc_variant_make_from_json_string(desc_for_unlabeled_app,
                sizeof(desc_for_unlabeled_app) - 1);
        if (fallback == PURC_VARIANT_INVALID) {
            goto failed;
        }
        purc_variant_object_set_by_ckey(manifest, KEY_DESC, fallback);
        purc_variant_unref(fallback);
    }
    else if (purc_variant_is_object(v)) {
        fallback = purc_variant_make_from_json_string(desc_for_unlabeled_app,
                sizeof(desc_for_unlabeled_app) - 1);
        if (fallback == PURC_VARIANT_INVALID) {
            goto failed;
        }
        purc_variant_object_unite(v, fallback, PCVRNT_CR_METHOD_IGNORE);
        purc_variant_unref(fallback);
    }

    v = purc_variant_object_get_by_ckey(manifest, KEY_ICON);
    if (v == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string(v) && !purc_variant_is_object(v))) {
        fallback = purc_variant_make_from_json_string(icon_for_unlabeled_app,
                sizeof(icon_for_unlabeled_app) - 1);
        if (fallback == PURC_VARIANT_INVALID) {
            goto failed;
        }
        purc_variant_object_set_by_ckey(manifest, KEY_ICON, fallback);
        purc_variant_unref(fallback);
    }
    else if (purc_variant_is_object(v)) {
        fallback = purc_variant_make_from_json_string(icon_for_unlabeled_app,
                sizeof(icon_for_unlabeled_app) - 1);
        if (fallback == PURC_VARIANT_INVALID) {
            goto failed;
        }
        purc_variant_object_unite(v, fallback, PCVRNT_CR_METHOD_IGNORE);
        purc_variant_unref(fallback);
    }

    return manifest;

failed:
    return PURC_VARIANT_INVALID;
}

purc_variant_t
purc_get_app_manifest(void)
{
    struct pcinst* inst = pcinst_current();
    if (inst == NULL)
        return PURC_VARIANT_INVALID;

    if (inst->app_manifest) {
        inst->app_manifest = pcinst_load_app_manifest(inst->app_name);
    }

    return inst->app_manifest;
}

static bool split_locale(const char *locale, char *lang, char *country_region)
{
    if (locale[0] < 'a' && locale[0] > 'z')
        return false;
    lang[0] = locale[0];

    if (locale[1] < 'a' && locale[1] > 'z')
        return false;
    lang[1] = locale[1];
    lang[2] = 0;

    if (locale[2] != '-' && locale[2] != '_')
        return false;

    if (locale[3] < 'A' && locale[3] > 'Z')
        return false;
    country_region[0] = locale[3];

    if (locale[4] < 'A' && locale[4] > 'Z')
        return false;
    country_region[1] = locale[4];
    country_region[2] = 0;

    return true;
}

static purc_variant_t get_app_manifest_via_key(const char *key,
        const char *prefix, const char *locale)
{
    purc_variant_t manifest, v;

    manifest = purc_get_app_manifest();
    if (manifest == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    v = purc_variant_object_get_by_ckey(manifest, key);
    assert(v);
    if (purc_variant_is_string(v))
        return v;

    assert(purc_variant_is_object(v));

    char lang[3], country_region[3];
    if (!split_locale(locale, lang, country_region)) {
        strcpy(lang, "en");
        strcpy(country_region, "US");
    }

    char subkey[(prefix?strlen(prefix):0) + 7];
    if (prefix) {
        strcpy(subkey, prefix);
        strcat(subkey, "-");
        strcat(subkey, lang);
    }
    else {
        strcpy(subkey, lang);
    }

    strcat(subkey, "_");
    strcat(subkey, country_region);

    purc_variant_t value;
    value = purc_variant_object_get_by_ckey(v, subkey);
    if (value)
        return value;

    if (prefix) {
        strcpy(subkey, prefix);
        strcat(subkey, "-");
        strcat(subkey, lang);
    }
    else {
        strcpy(subkey, lang);
    }

    value = purc_variant_object_get_by_ckey(v, subkey);
    if (value)
        return value;

    /* fallback */
    if (prefix) {
        value = purc_variant_object_get_by_ckey(v, prefix);
        if (value == PURC_VARIANT_INVALID)
            value = purc_variant_object_get_by_ckey(v, FALLBACK_DENSITY);
    }
    else {
        value = purc_variant_object_get_by_ckey(v, "en");
    }

    assert(value);
    return value;
}

purc_variant_t
purc_get_app_label(const char *locale)
{
    return get_app_manifest_via_key(KEY_LABEL, NULL, locale);
}

purc_variant_t
purc_get_app_description(const char *locale)
{
    return get_app_manifest_via_key(KEY_DESC, NULL, locale);
}

#include "hvml_v_png.inc"

purc_variant_t
purc_get_app_icon_content(const char *display_density, const char *locale)
{
    purc_variant_t v;
    v = get_app_manifest_via_key(KEY_ICON, display_density, locale);
    if (v == PURC_VARIANT_INVALID) {
        return v;
    }

    if (purc_variant_is_null(v)) {
        goto fallback;
    }
    else {
        const char *file = purc_variant_get_string_const(v);
        assert(file);

        char *strp = NULL;
        if (file[0] != '/') {
            struct pcinst* inst = pcinst_current();
            assert(inst);

            int n = asprintf(&strp, PURC_PATH_APP_FILE, inst->app_name, file);
            if (n > 0) {
                file = strp;
            }
            else
                goto fallback;
        }

        char *content;
        size_t sz;
        content = purc_load_file_contents(file, &sz);
        if (strp)
            free(strp);

        if (content)
            return purc_variant_make_byte_sequence_reuse_buff(content, sz, sz);
    }

fallback:
    return purc_variant_make_byte_sequence_static(hvml_v_png_data,
                sizeof(hvml_v_png_data));
}

