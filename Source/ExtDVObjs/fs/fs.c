/*
 * @file fs.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of file system dynamic variant object.
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

#include "config.h"
#include "private/instance.h"
#include "private/errors.h"
#include "private/dvobjs.h"
#include "purc-variant.h"

#if HAVE(SYS_SYSMACROS_H)
#include <sys/sysmacros.h>
#endif

#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>

#if USE(GLIB)
#include <glib.h>
#endif

#define FS_DVOBJ_VERSION    0

purc_variant_t pcdvobjs_create_file (void);
typedef purc_variant_t (*pcdvobjs_create) (void);

// as FILE, FS, MATH
struct pcdvobjs_dvobjs_object {
    const char *name;
    const char *description;
    pcdvobjs_create create_func;
};

// dynamic variant in dynamic object
struct pcdvobjs_dvobjs {
    const char *name;
    purc_dvariant_method getter;
    purc_dvariant_method setter;
};

static const char * pcdvobjs_get_next_option (const char *data, 
        const char *delims, size_t *length)
{
    const char *head = data;
    char *temp = NULL;

    if ((delims == NULL) || (data == NULL) || (*delims == 0x00))
        return NULL;

    *length = 0;

    while (*data != 0x00) {
        temp = strchr (delims, *data);
        if (temp) {
            if (head == data) {
                head = data + 1;
            }
            else
                break;
        }
        data++;
    }

    *length = data - head;
    if (*length == 0)
        head = NULL;

    return head;
}

static const char * pcdvobjs_remove_space (char *buffer)
{
    int i = 0;
    int j = 0;
    while (*(buffer + i) != 0x00) {
        if (*(buffer + i) != ' ') {
            *(buffer + j) = *(buffer + i);
            j++;
        }
        i++;
    }
    *(buffer + j) = 0x00;

    return buffer;
}

#if USE(GLIB)
static bool wildcard_cmp (const char *str1, const char *pattern)
{
    GPatternSpec *glib_pattern = g_pattern_spec_new (pattern);

    gboolean result = g_pattern_match_string (glib_pattern, str1);

    g_pattern_spec_free (glib_pattern);

    return (bool)result;
}
#else
static bool wildcard_cmp (const char *str1, const char *pattern)
{
    if (str1 == NULL)
        return false;
    if (pattern == NULL)
        return false;

    int len1 = strlen (str1);
    int len2 = strlen (pattern);
    int mark = 0;
    int p1 = 0;
    int p2 = 0;

    while ((p1 < len1) && (p2<len2)) {
        if (pattern[p2] == '?') {
            p1++;
            p2++;
            continue;
        }
        if (pattern[p2] == '*') {
            p2++;
            mark = p2;
            continue;
        }
        if (str1[p1] != pattern[p2]) {
            if (p1 == 0 && p2 == 0)
                return false;
            p1 -= p2 - mark - 1;
            p2 = mark;
            continue;
        }
        p1++;
        p2++;
    }
    if (p2 == len2) {
        if (p1 == len1)
            return true;
        if (pattern[p2 - 1] == '*')
            return true;
    }
    while (p2 < len2) {
        if (pattern[p2] != '*')
            return false;
        p2++;
    }
    return true;
}
#endif

purc_variant_t pcdvobjs_make_dvobjs (
        const struct pcdvobjs_dvobjs *method, size_t size)
{
    size_t i = 0;
    purc_variant_t val = PURC_VARIANT_INVALID;
    purc_variant_t ret_var= purc_variant_make_object (0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);

    if (ret_var == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    for (i = 0; i < size; i++) {
        val = purc_variant_make_dynamic (method[i].getter, method[i].setter);
        if (val == PURC_VARIANT_INVALID) {
            goto error;
        }

        if (!purc_variant_object_set_by_static_ckey (ret_var,
                    method[i].name, val)) {
            goto error;
        }

        purc_variant_unref (val);
    }

    return ret_var;

error:
    purc_variant_unref (ret_var);
    return PURC_VARIANT_INVALID;
}

static bool remove_dir (char *dir)
{
    char dir_name[PATH_MAX];
    DIR *dirp;
    struct dirent *dp;
    struct stat dir_stat;
    bool ret = true;

    if (access(dir, F_OK | R_OK) != 0)
        return false;

    if (stat(dir, &dir_stat) < 0)
        return false;

    if (S_ISREG(dir_stat.st_mode))
        remove(dir);
    else if (S_ISDIR(dir_stat.st_mode)) {
        dirp = opendir(dir);

        while ((dp=readdir(dirp)) != NULL) {
            if ((strcmp(dp->d_name, ".") == 0)
                    || (strcmp(dp->d_name, "..") == 0))
                continue;
            sprintf(dir_name, "%s/%s", dir, dp->d_name);
            remove_dir(dir_name);
        }
        closedir(dirp);

        rmdir(dir);
    }
    else
        ret = false;

    return ret;
}

static purc_variant_t
list_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    char dir_name[PATH_MAX];
    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;
    const char *filter = NULL;
    struct wildcard_list *wildcard = NULL;
    struct wildcard_list *temp_wildcard = NULL;
    char au[10] = {0};
    int i = 0;

    if ((argv == NULL) || (nr_args < 1)) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strcpy (dir_name, string_filename);

    if (access(dir_name, F_OK | R_OK) != 0) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the filter
    if ((nr_args > 1) && (argv[1] != NULL) &&
            (!purc_variant_is_string (argv[1]))) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }
    if ((nr_args > 1) && (argv[1] != NULL))
        filter = purc_variant_get_string_const (argv[1]);

    // get filter array
    if (filter) {
        size_t length = 0;
        const char *head = pcdvobjs_get_next_option (filter, ";", &length);
        while (head) {
            if (wildcard == NULL) {
                wildcard = malloc (sizeof(struct wildcard_list));
                if (wildcard == NULL)
                    goto error;
                temp_wildcard = wildcard;
            }
            else {
                temp_wildcard->next = malloc (sizeof(struct wildcard_list));
                if (temp_wildcard->next == NULL)
                    goto error;
                temp_wildcard = temp_wildcard->next;
            }
            temp_wildcard->next = NULL;
            temp_wildcard->wildcard = malloc (length + 1);
            if (temp_wildcard->wildcard == NULL)
                goto error;
            strncpy(temp_wildcard->wildcard, head, length);
            *(temp_wildcard->wildcard + length) = 0x00;
            pcdvobjs_remove_space (temp_wildcard->wildcard);
            head = pcdvobjs_get_next_option (head + length + 1, ";", &length);
        }
    }

    // get the dirctory content
    DIR *dir = NULL;
    struct dirent *ptr = NULL;
    purc_variant_t obj_var = PURC_VARIANT_INVALID;
    struct stat file_stat;

    if ((dir = opendir (dir_name)) == NULL) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        goto error;
    }

    ret_var = purc_variant_make_array (0, PURC_VARIANT_INVALID);
    while ((ptr = readdir(dir)) != NULL)
    {
        if (strcmp (ptr->d_name,".") == 0 || strcmp(ptr->d_name, "..") == 0)
            continue;

        // use filter
        temp_wildcard = wildcard;
        while (temp_wildcard) {
            if (wildcard_cmp (ptr->d_name, temp_wildcard->wildcard))
                break;
            temp_wildcard = temp_wildcard->next;
        }
        if (wildcard && (temp_wildcard == NULL))
            continue;

        obj_var = purc_variant_make_object (0, PURC_VARIANT_INVALID,
                PURC_VARIANT_INVALID);

        strcpy (filename, dir_name);
        strcat (filename, "/");
        strcat (filename, ptr->d_name);

        if (stat(filename, &file_stat) < 0)
            continue;

        // name
        val = purc_variant_make_string (ptr->d_name, false);
        purc_variant_object_set_by_static_ckey (obj_var, "name", val);
        purc_variant_unref (val);

        // dev
        val = purc_variant_make_number (file_stat.st_dev);
        purc_variant_object_set_by_static_ckey (obj_var, "dev", val);
        purc_variant_unref (val);

        // inode
        val = purc_variant_make_number (ptr->d_ino);
        purc_variant_object_set_by_static_ckey (obj_var, "inode", val);
        purc_variant_unref (val);

        // type
        if (ptr->d_type == DT_BLK) {
            val = purc_variant_make_string ("b", false);
            purc_variant_object_set_by_static_ckey (obj_var, "type", val);
            purc_variant_unref (val);
        }
        else if(ptr->d_type == DT_CHR) {
            val = purc_variant_make_string ("c", false);
            purc_variant_object_set_by_static_ckey (obj_var, "type", val);
            purc_variant_unref (val);
        }
        else if(ptr->d_type == DT_DIR) {
            val = purc_variant_make_string ("d", false);
            purc_variant_object_set_by_static_ckey (obj_var, "type", val);
            purc_variant_unref (val);
        }
        else if(ptr->d_type == DT_FIFO) {
            val = purc_variant_make_string ("f", false);
            purc_variant_object_set_by_static_ckey (obj_var, "type", val);
            purc_variant_unref (val);
        }
        else if(ptr->d_type == DT_LNK) {
            val = purc_variant_make_string ("l", false);
            purc_variant_object_set_by_static_ckey (obj_var, "type", val);
            purc_variant_unref (val);
        }
        else if(ptr->d_type == DT_REG) {
            val = purc_variant_make_string ("r", false);
            purc_variant_object_set_by_static_ckey (obj_var, "type", val);
            purc_variant_unref (val);
        }
        else if(ptr->d_type == DT_SOCK) {
            val = purc_variant_make_string ("s", false);
            purc_variant_object_set_by_static_ckey (obj_var, "type", val);
            purc_variant_unref (val);
        }
        else if(ptr->d_type == DT_UNKNOWN) {
            val = purc_variant_make_string ("u", false);
            purc_variant_object_set_by_static_ckey (obj_var, "type", val);
            purc_variant_unref (val);
        }

        // mode
        val = purc_variant_make_byte_sequence (&(file_stat.st_mode),
                                                    sizeof(unsigned long));
        purc_variant_object_set_by_static_ckey (obj_var, "mode", val);
        purc_variant_unref (val);

        // mode_str
        for (i = 0; i < 3; i++) {
            if ((0x01 << (8 - 3 * i)) & file_stat.st_mode)
                au[i * 3 + 0] = 'r';
            else
                au[i * 3 + 0] = '-';
            if ((0x01 << (7 - 3 * i)) & file_stat.st_mode)
                au[i * 3 + 1] = 'w';
            else
                au[i * 3 + 1] = '-';
            if ((0x01 << (6 - 3 * i)) & file_stat.st_mode)
                au[i * 3 + 2] = 'x';
            else
                au[i * 3 + 2] = '-';
        }
        val = purc_variant_make_string (au, false);
        purc_variant_object_set_by_static_ckey (obj_var, "mode_str", val);
        purc_variant_unref (val);

        // nlink
        val = purc_variant_make_number (file_stat.st_nlink);
        purc_variant_object_set_by_static_ckey (obj_var, "nlink", val);
        purc_variant_unref (val);

        // uid
        val = purc_variant_make_number (file_stat.st_uid);
        purc_variant_object_set_by_static_ckey (obj_var, "uid", val);
        purc_variant_unref (val);

        // gid
        val = purc_variant_make_number (file_stat.st_gid);
        purc_variant_object_set_by_static_ckey (obj_var, "gid", val);
        purc_variant_unref (val);

        // rdev_major 
        val = purc_variant_make_number (major(file_stat.st_dev));
        purc_variant_object_set_by_static_ckey (obj_var, "rdev_major", val);
        purc_variant_unref (val);

        // rdev_minor
        val = purc_variant_make_number (minor(file_stat.st_dev));
        purc_variant_object_set_by_static_ckey (obj_var, "rdev_minor", val);
        purc_variant_unref (val);

        // size
        val = purc_variant_make_number (file_stat.st_size);
        purc_variant_object_set_by_static_ckey (obj_var, "size", val);
        purc_variant_unref (val);

        // blksize
        val = purc_variant_make_number (file_stat.st_blksize);
        purc_variant_object_set_by_static_ckey (obj_var, "blksize", val);
        purc_variant_unref (val);

        // blocks
        val = purc_variant_make_number (file_stat.st_blocks);
        purc_variant_object_set_by_static_ckey (obj_var, "blocks", val);
        purc_variant_unref (val);

        // atime
        val = purc_variant_make_string (ctime(&file_stat.st_atime), false);
        purc_variant_object_set_by_static_ckey (obj_var, "atime", val);
        purc_variant_unref (val);

        // mtime
        val = purc_variant_make_string (ctime(&file_stat.st_mtime), false);
        purc_variant_object_set_by_static_ckey (obj_var, "mtime", val);
        purc_variant_unref (val);

        // ctime
        val = purc_variant_make_string (ctime(&file_stat.st_ctime), false);
        purc_variant_object_set_by_static_ckey (obj_var, "ctime", val);
        purc_variant_unref (val);

        purc_variant_array_append (ret_var, obj_var);
        purc_variant_unref (obj_var);
    }

    closedir(dir);

error:
    while (wildcard) {
        if (wildcard->wildcard)
            free (wildcard->wildcard);
        temp_wildcard = wildcard;
        wildcard = wildcard->next;
        free (temp_wildcard);
    }
    return ret_var;
}

static purc_variant_t
list_prt_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    enum display_order {
        DISPLAY_MODE = 1,
        DISPLAY_NLINK,
        DISPLAY_UID,
        DISPLAY_GID,
        DISPLAY_SIZE,
        DISPLAY_BLKSIZE,
        DISPLAY_ATIME,
        DISPLAY_CTIME,
        DISPLAY_MTIME,
        DISPLAY_NAME,
        DISPLAY_MAX
    };
    char dir_name[PATH_MAX];
    char filename[PATH_MAX];
    const char *string_filename = NULL;
    const char *filter = NULL;
    struct wildcard_list *wildcard = NULL;
    struct wildcard_list *temp_wildcard = NULL;
    const char *mode = NULL;
    char display[DISPLAY_MAX] = {0};
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;
    char au[10] = {0};
    int i = 0;

    if ((argv == NULL) || (nr_args < 1)) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strcpy (dir_name, string_filename);

    if (access(dir_name, F_OK | R_OK) != 0) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the filter
    if ((nr_args > 1) && (argv[1] != NULL) &&
            (!purc_variant_is_string (argv[1]))) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }
    if ((nr_args > 1) && (argv[1] != NULL))
        filter = purc_variant_get_string_const (argv[1]);

    // get filter array
    if (filter) {
        size_t length = 0;
        const char *head = pcdvobjs_get_next_option (filter, ";", &length);
        while (head) {
            if (wildcard == NULL) {
                wildcard = malloc (sizeof(struct wildcard_list));
                if (wildcard == NULL)
                    goto error;
                temp_wildcard = wildcard;
            }
            else {
                temp_wildcard->next = malloc (sizeof(struct wildcard_list));
                if (temp_wildcard->next == NULL)
                    goto error;
                temp_wildcard = temp_wildcard->next;
            }
            temp_wildcard->next = NULL;
            temp_wildcard->wildcard = malloc (length + 1);
            if (temp_wildcard->wildcard == NULL)
                goto error;
            strncpy(temp_wildcard->wildcard, head, length);
            *(temp_wildcard->wildcard + length) = 0x00;
            pcdvobjs_remove_space (temp_wildcard->wildcard);
            head = pcdvobjs_get_next_option (head + length + 1, ";", &length);
        }
    }

    // get the mode
    if ((nr_args > 2) && (argv[2] != NULL) && (!purc_variant_is_string (argv[2]))) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        goto error;
    }
    if ((nr_args > 2) && (argv[2] != NULL)) {
        mode = purc_variant_get_string_const (argv[2]);

        // get mode array
        i = 0;
        bool quit = false;
        size_t length = 0;
        const char * head = pcdvobjs_get_next_option (mode, " ", &length);
        while (head) {
            switch (* head)
            {
                case 'm':
                case 'M':
                    if (strncasecmp (head, "mode", length) == 0) {
                        display[i] = DISPLAY_MODE;
                        i++;
                    }
                    else if (strncasecmp (head, "mtime", length) == 0) {
                        display[i] = DISPLAY_MTIME;
                        i++;
                    }
                    break;
                case 'n':
                case 'N':
                    if (strncasecmp (head, "nlink", length) == 0) {
                        display[i] = DISPLAY_NLINK;
                        i++;
                    }
                    else if (strncasecmp (head, "name", length) == 0) {
                        display[i] = DISPLAY_NAME;
                        i++;
                    }
                    break;
                case 'u':
                case 'U':
                    if (strncasecmp (head, "uid", length) == 0) {
                        display[i] = DISPLAY_UID;
                        i++;
                    }
                    break;
                case 'g':
                case 'G':
                    if (strncasecmp (head, "gid", length) == 0) {
                        display[i] = DISPLAY_GID;
                        i++;
                    }
                    break;
                case 's':
                case 'S':
                    if (strncasecmp (head, "size", length) == 0) {
                        display[i] = DISPLAY_SIZE;
                        i++;
                    }
                    break;
                case 'b':
                case 'B':
                    if (strncasecmp (head, "blksize", length) == 0) {
                        display[i] = DISPLAY_BLKSIZE;
                        i++;
                    }
                    break;
                case 'a':
                case 'A':
                    if (strncasecmp (head, "atime", length) == 0) {
                        display[i] = DISPLAY_ATIME;
                        i++;
                    }
                    else if (strncasecmp (head, "all", length) == 0) {
                        for (i = 0; i < (DISPLAY_MAX - 1); i++)
                            display[i] = i + 1;
                        quit = true;
                    }
                    break;
                case 'c':
                case 'C':
                    if (strncasecmp (head, "ctime", length) == 0) {
                        display[i] = DISPLAY_CTIME;
                        i++;
                    }
                    break;
                case 'd':
                case 'D':
                    if (strncasecmp (head, "default", length) == 0) {
                        for (i = 0; i < (DISPLAY_MAX - 1); i++)
                            display[i] = i + 1;
                        quit = true;
                    }
                    break;
            }

            if (quit)
                break;
            head = pcdvobjs_get_next_option (head + length + 1, " ", &length);
        }
    }
    else {
        for (i = 0; i < (DISPLAY_MAX - 1); i++)
            display[i] = i + 1;
    }

    // get the dirctory content
    DIR *dir = NULL;
    struct dirent *ptr = NULL;
    struct stat file_stat;
    char info[PATH_MAX] = {0};

    if ((dir = opendir (dir_name)) == NULL) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        goto error;
    }

    ret_var = purc_variant_make_array (0, PURC_VARIANT_INVALID);
    while ((ptr = readdir(dir)) != NULL)
    {
        if (strcmp (ptr->d_name,".") == 0 || strcmp(ptr->d_name, "..") == 0)
            continue;

        // use filter
        temp_wildcard = wildcard;
        while (temp_wildcard) {
            if (wildcard_cmp (ptr->d_name, temp_wildcard->wildcard))
                break;
            temp_wildcard = temp_wildcard->next;
        }
        if (wildcard && (temp_wildcard == NULL))
            continue;

        strcpy (filename, dir_name);
        strcat (filename, "/");
        strcat (filename, ptr->d_name);

        if (stat(filename, &file_stat) < 0)
            continue;

        for (i = 0; i < (DISPLAY_MAX - 1); i++) {
            switch (display[i]) {
                case DISPLAY_MODE:
                    // type
                    if (ptr->d_type == DT_BLK) {
                        sprintf (info + strlen (info), "b");
                    }
                    else if(ptr->d_type == DT_CHR) {
                        sprintf (info + strlen (info), "c");
                    }
                    else if(ptr->d_type == DT_DIR) {
                        sprintf (info + strlen (info), "d");
                    }
                    else if(ptr->d_type == DT_FIFO) {
                        sprintf (info + strlen (info), "f");
                    }
                    else if(ptr->d_type == DT_LNK) {
                        sprintf (info + strlen (info), "l");
                    }
                    else if(ptr->d_type == DT_REG) {
                        sprintf (info + strlen (info), "-");
                    }
                    else if(ptr->d_type == DT_SOCK) {
                        sprintf (info + strlen (info), "s");
                    }

                    // mode_str
                    for (i = 0; i < 3; i++) {
                        if ((0x01 << (8 - 3 * i)) & file_stat.st_mode)
                            au[i * 3 + 0] = 'r';
                        else
                            au[i * 3 + 0] = '-';
                        if ((0x01 << (7 - 3 * i)) & file_stat.st_mode)
                            au[i * 3 + 1] = 'w';
                        else
                            au[i * 3 + 1] = '-';
                        if ((0x01 << (6 - 3 * i)) & file_stat.st_mode)
                            au[i * 3 + 2] = 'x';
                        else
                            au[i * 3 + 2] = '-';
                    }
                    sprintf (info + strlen (info), "%s\t", au);
                    break;

                case DISPLAY_NLINK:
                    sprintf (info + strlen (info), "%ld\t",
                            (long)file_stat.st_nlink);
                    break;

                case DISPLAY_UID:
                    sprintf (info + strlen (info), "%ld\t",
                            (long)file_stat.st_uid);
                    break;

                case DISPLAY_GID:
                    sprintf (info + strlen (info), "%ld\t",
                            (long)file_stat.st_gid);
                    break;

                case DISPLAY_SIZE:
                    sprintf (info + strlen (info), "%llu\t",
                            (long long unsigned)file_stat.st_size);
                    break;

                case DISPLAY_BLKSIZE:
                    sprintf (info + strlen (info), "%llu\t",
                            (long long unsigned)file_stat.st_blksize);
                    break;

                case DISPLAY_ATIME:
                    sprintf (info + strlen (info), "%s\t",
                            ctime(&file_stat.st_atime));
                    break;

                case DISPLAY_CTIME:
                    sprintf (info + strlen (info), "%s\t",
                            ctime(&file_stat.st_ctime));
                    break;

                case DISPLAY_MTIME:
                    sprintf (info + strlen (info), "%s\t",
                            ctime(&file_stat.st_mtime));
                    break;

                case DISPLAY_NAME:
                    strcat (info, ptr->d_name);
                    strcat (info, "\t");
                    break;
            }
        }
        info[strlen (info) - 1] = 0x00;

        val = purc_variant_make_string (info, false);
        purc_variant_array_append (ret_var, val);
        purc_variant_unref (val);
    }

    closedir(dir);

error:
    while (wildcard) {
        if (wildcard->wildcard)
            free (wildcard->wildcard);
        temp_wildcard = wildcard;
        wildcard = wildcard->next;
        free (temp_wildcard);
    }
    return ret_var;
}

static purc_variant_t
mkdir_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    const char *filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args != 1)) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);

    if (mkdir (filename, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0)
        ret_var = purc_variant_make_boolean (false);
    else
        ret_var = purc_variant_make_boolean (true);

    return ret_var;
}


static purc_variant_t
rmdir_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    const char *filename = NULL;
    DIR *dirp;
    struct dirent *dp;
    struct stat dir_stat;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    bool empty = true;

    if ((argv == NULL) || (nr_args != 1)) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);

    if (access(filename, F_OK | R_OK) != 0) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return purc_variant_make_boolean (false);
    }

    if (stat(filename, &dir_stat) < 0)
        return purc_variant_make_boolean (false);

    if (S_ISDIR(dir_stat.st_mode)) {
        dirp = opendir(filename);

        while ((dp=readdir(dirp)) != NULL) {
            if ((strcmp(dp->d_name, ".") == 0) ||
                    (strcmp(dp->d_name, "..") == 0))
                continue;
            else {
                empty = false;
                break;
            }
        }
        closedir(dirp);

        if (empty) {
            if (rmdir(filename) == 0)
                empty = true;
            else
                empty = false;
        }
    }

    if (empty)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
touch_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    const char *filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args != 1)) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);

    // file not exist, create it
    if (access(filename, F_OK | R_OK) != 0) {
        int fd = -1;
        fd = open(filename, O_CREAT | O_WRONLY,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH |S_IWOTH);

        if (fd != -1) {
            ret_var = purc_variant_make_boolean (true);
            close (fd);
        }
        else
            ret_var = purc_variant_make_boolean (false);
    }
    else {      // change time
        struct timespec newtime[2];
        newtime[0].tv_nsec = UTIME_NOW;
        newtime[1].tv_nsec = UTIME_NOW;
        if (utimensat(AT_FDCWD, filename, newtime, 0) == 0) {
            ret_var = purc_variant_make_boolean (true);
        }
        else {
            ret_var = purc_variant_make_boolean (false);
        }
    }

    return ret_var;
}


static purc_variant_t
unlink_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    const char *filename = NULL;
    struct stat filestat;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args != 1)) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);

    if (access(filename, F_OK | R_OK) != 0) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return purc_variant_make_boolean (false);
    }

    if (stat(filename, &filestat) < 0)
        return purc_variant_make_boolean (false);

    if (S_ISREG(filestat.st_mode)) {
        if (unlink (filename) == 0)
            ret_var = purc_variant_make_boolean (false);
        else
            ret_var = purc_variant_make_boolean (true);
    }
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
rm_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    const char *filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args != 1)) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);

    if (remove_dir ((char *)filename))
        ret_var = purc_variant_make_boolean (false);
    else
        ret_var = purc_variant_make_boolean (true);

    return ret_var;

}

static purc_variant_t pcdvobjs_create_fs(void)
{
    static struct pcdvobjs_dvobjs method [] = {
        {"list",     list_getter, NULL},
        {"list_prt", list_prt_getter, NULL},
        {"mkdir",    mkdir_getter, NULL},
        {"rmdir",    rmdir_getter, NULL},
        {"touch",    touch_getter, NULL},
        {"unlink",   unlink_getter, NULL},
        {"rm",       rm_getter, NULL} };

    return pcdvobjs_make_dvobjs (method, PCA_TABLESIZE(method));
}

static struct pcdvobjs_dvobjs_object dynamic_objects [] = {
    {
        "FS",                                   // name
        "For File System Operations in PURC",   // description
        pcdvobjs_create_fs                      // create function
    },
    {
        "FILE",                                 // name
        "For File Operations in PURC",          // description
        pcdvobjs_create_file                    // create function
    }
};

purc_variant_t __purcex_load_dynamic_variant (const char *name, int *ver_code)
{
    size_t i = 0;
    for (i = 0; i < PCA_TABLESIZE(dynamic_objects); i++) {
        if (strncasecmp (name, dynamic_objects[i].name, strlen (name)) == 0)
            break;
    }

    if (i == PCA_TABLESIZE(dynamic_objects))
        return PURC_VARIANT_INVALID;
    else {
        *ver_code = FS_DVOBJ_VERSION;
        return dynamic_objects[i].create_func();
    }
}

size_t __purcex_get_number_of_dynamic_variants (void)
{
    return PCA_TABLESIZE(dynamic_objects);
}

const char * __purcex_get_dynamic_variant_name (size_t idx)
{
    if (idx >= PCA_TABLESIZE(dynamic_objects))
        return NULL;
    else
        return dynamic_objects[idx].name;
}

const char * __purcex_get_dynamic_variant_desc (size_t idx)
{
    if (idx >= PCA_TABLESIZE(dynamic_objects))
        return NULL;
    else
        return dynamic_objects[idx].description;
}
