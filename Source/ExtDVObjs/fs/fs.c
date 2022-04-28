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
#include <pwd.h>
#include <errno.h>
#include <stdlib.h>

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

#if GLIB_CHECK_VERSION(2, 70, 0)
    gboolean result = g_pattern_spec_match_string (glib_pattern, str1);
#else
    gboolean result = g_pattern_match_string (glib_pattern, str1);
#endif

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

        while ((dp = readdir(dirp)) != NULL) {
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

void set_purc_error_by_errno (void)
{
    switch (errno)
    {
        case EACCES:
            purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
            break;

        case ENOENT:
            purc_set_error (PURC_ERROR_ENTITY_NOT_FOUND);
            break;

        case ENAMETOOLONG:
            purc_set_error (PURC_ERROR_TOO_LONG);
            break;

        case EIO:
            purc_set_error (PURC_ERROR_SYSTEM_FAULT);
            break;

        default:
            purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
    }
}

// Transmit 
#define INVALID_MODE  (unsigned int)-1
unsigned int str_to_mode (const char *string_mode)
{
    UNUSED_PARAM(string_mode);
    
    // wait for code
    return INVALID_MODE; // error
}

static purc_variant_t
list_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

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

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (NULL == string_filename) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    strncpy (dir_name, string_filename, sizeof(dir_name));

    if (access(dir_name, F_OK | R_OK) != 0) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }

    // get the filter
    if ((nr_args > 1) && (argv[1] == NULL ||
            (!purc_variant_is_string (argv[1])))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    if ((nr_args > 1) && (argv[1] != NULL))
        filter = purc_variant_get_string_const (argv[1]);

    // get filter array
    if (filter) {
        size_t length = 0;
        const char *head = pcutils_get_next_token (filter, ";", &length);
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
            head = pcutils_get_next_token (head + length + 1, ";", &length);
        }
    }

    // get the dirctory content
    DIR *dir = NULL;
    struct dirent *ptr = NULL;
    purc_variant_t obj_var = PURC_VARIANT_INVALID;
    struct stat file_stat;

    if ((dir = opendir (dir_name)) == NULL) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
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

        strncpy (filename, dir_name, sizeof(filename));
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
list_prt_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

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

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (NULL == string_filename) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    strncpy (dir_name, string_filename, sizeof(dir_name));

    if (access(dir_name, F_OK | R_OK) != 0) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }

    // get the filter
    if ((nr_args > 1) && (argv[1] != PURC_VARIANT_INVALID &&
            (!purc_variant_is_string (argv[1])))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    if ((nr_args > 1) && (argv[1] != NULL))
        filter = purc_variant_get_string_const (argv[1]);

    // get filter array
    if (filter) {
        size_t length = 0;
        const char *head = pcutils_get_next_token (filter, ";", &length);
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
            head = pcutils_get_next_token (head + length + 1, ";", &length);
        }
    }

    // get the mode
    if ((nr_args > 2) && (argv[2] == NULL || (!purc_variant_is_string (argv[2])))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }
    if ((nr_args > 2) && (argv[2] != NULL)) {
        mode = purc_variant_get_string_const (argv[2]);

        // get mode array
        i = 0;
        bool quit = false;
        size_t length = 0;
        const char * head = pcutils_get_next_token (mode, " ", &length);
        while (head) {
            switch (* head)
            {
                case 'm':
                case 'M':
                    if (pcutils_strncasecmp (head, "mode", length) == 0) {
                        display[i] = DISPLAY_MODE;
                        i++;
                    }
                    else if (pcutils_strncasecmp (head, "mtime", length) == 0) {
                        display[i] = DISPLAY_MTIME;
                        i++;
                    }
                    break;
                case 'n':
                case 'N':
                    if (pcutils_strncasecmp (head, "nlink", length) == 0) {
                        display[i] = DISPLAY_NLINK;
                        i++;
                    }
                    else if (pcutils_strncasecmp (head, "name", length) == 0) {
                        display[i] = DISPLAY_NAME;
                        i++;
                    }
                    break;
                case 'u':
                case 'U':
                    if (pcutils_strncasecmp (head, "uid", length) == 0) {
                        display[i] = DISPLAY_UID;
                        i++;
                    }
                    break;
                case 'g':
                case 'G':
                    if (pcutils_strncasecmp (head, "gid", length) == 0) {
                        display[i] = DISPLAY_GID;
                        i++;
                    }
                    break;
                case 's':
                case 'S':
                    if (pcutils_strncasecmp (head, "size", length) == 0) {
                        display[i] = DISPLAY_SIZE;
                        i++;
                    }
                    break;
                case 'b':
                case 'B':
                    if (pcutils_strncasecmp (head, "blksize", length) == 0) {
                        display[i] = DISPLAY_BLKSIZE;
                        i++;
                    }
                    break;
                case 'a':
                case 'A':
                    if (pcutils_strncasecmp (head, "atime", length) == 0) {
                        display[i] = DISPLAY_ATIME;
                        i++;
                    }
                    else if (pcutils_strncasecmp (head, "all", length) == 0) {
                        for (i = 0; i < (DISPLAY_MAX - 1); i++)
                            display[i] = i + 1;
                        quit = true;
                    }
                    break;
                case 'c':
                case 'C':
                    if (pcutils_strncasecmp (head, "ctime", length) == 0) {
                        display[i] = DISPLAY_CTIME;
                        i++;
                    }
                    break;
                case 'd':
                case 'D':
                    if (pcutils_strncasecmp (head, "default", length) == 0) {
                        for (i = 0; i < (DISPLAY_MAX - 1); i++)
                            display[i] = i + 1;
                        quit = true;
                    }
                    break;
            }

            if (quit)
                break;
            head = pcutils_get_next_token (head + length + 1, " ", &length);
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
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
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

        strncpy (filename, dir_name, sizeof(filename));
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
basename_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    // On Linux, slash (/) is used as directory separator character.
    const char separator = '/';
    const char *string_path = NULL;
    const char *string_suffix = NULL;
    const char *base_begin = NULL;
    const char *temp_ptr = NULL;
    const char *base_end = NULL;
    purc_variant_t ret_string = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the parameters
    string_path = purc_variant_get_string_const (argv[0]);
    if (NULL == string_path) {
        ret_string = purc_variant_make_string("", true);
        return ret_string;
    }
    if (nr_args > 1) {
        string_suffix = purc_variant_get_string_const (argv[1]);
    }

    // Mark out the trailing name component.
    base_begin = string_path;
    temp_ptr = base_begin;
    base_end = base_begin + strlen(string_path);

    while (base_end > base_begin && separator == *(base_end - 1)) {
        base_end--;
    }

    while (temp_ptr < base_end) {
        if (separator == *temp_ptr) {
            base_begin = temp_ptr + 1;
        }
        temp_ptr ++;
    }

    // If the name component ends in suffix this will also be cut off.
    if (string_suffix) {
        int suffix_len = strlen(string_suffix);
        temp_ptr = base_end - suffix_len;
        if (temp_ptr > base_begin &&
            0 == strncmp(string_suffix, temp_ptr, suffix_len)) {
            base_end = temp_ptr;
        }
    }

    ret_string = purc_variant_make_string_ex(base_begin,
            (base_end - base_begin), true);
    return ret_string;
}

static purc_variant_t
chgrp_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    const char *filename = NULL;
    const char *string_group = NULL;
    gid_t gid;
    struct passwd *pwd;
    char *endptr;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 2) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name and group name (or gid)
    filename = purc_variant_get_string_const (argv[0]);
    string_group = purc_variant_get_string_const (argv[1]);
    if (NULL == filename || NULL == string_group) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    // group : string_name | gid
    gid = strtol(string_group, &endptr, 10);  /* Allow a numeric string */

    if (*endptr != '\0') {              /* Was not pure numeric string */
        pwd = getpwnam(string_group);   /* Try getting GID for username */
        if (pwd == NULL) {
            purc_set_error (PURC_ERROR_BAD_NAME);
            return PURC_VARIANT_INVALID;
        }

        gid = pwd->pw_gid;
    }

    // If the owner or group is specified as -1, then that ID is not changed.
    if (0 == chown (filename, -1, gid)) {
        ret_var = purc_variant_make_boolean (true);
    }
    else {
        set_purc_error_by_errno ();
        ret_var = purc_variant_make_boolean (false);
    }

    return ret_var;
}

static purc_variant_t
chmod_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    const char *filename = NULL;
    const char *string_mode = NULL;
    mode_t new_mode;
    char *endptr;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 2) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name and the mode
    filename = purc_variant_get_string_const (argv[0]);
    string_mode = purc_variant_get_string_const (argv[1]);
    if (NULL == filename || NULL == string_mode) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    // new_mode : string_mode | int (octal | decimal)
    if ('0' == string_mode[0]) {
        new_mode = strtol (string_mode, &endptr, 8);  /* Octal number */
    }
    else {
        new_mode = strtol (string_mode, &endptr, 10); /* Decimal */
    }

    if (*endptr != '\0') {              /* Was not pure numeric string */
        new_mode = str_to_mode (string_mode);
        if (INVALID_MODE == new_mode) {
            purc_set_error (PURC_ERROR_BAD_NAME);
            return PURC_VARIANT_INVALID;
        }
    }

    if (0 == chmod (filename, new_mode)) {
        ret_var = purc_variant_make_boolean (true);
    }
    else {
        set_purc_error_by_errno ();
        ret_var = purc_variant_make_boolean (false);
    }

    return ret_var;
}

static purc_variant_t
chown_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    const char *filename = NULL;
    const char *string_owner = NULL;
    uid_t uid;
    struct passwd *pwd;
    char *endptr;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 2) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name and group name (or gid)
    filename = purc_variant_get_string_const (argv[0]);
    string_owner = purc_variant_get_string_const (argv[1]);
    if (NULL == filename || NULL == string_owner) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    // owner : string_name | uid
    uid = strtol (string_owner, &endptr, 10);  /* Allow a numeric string */

    if (*endptr != '\0') {              /* Was not pure numeric string */
        pwd = getpwnam (string_owner);   /* Try getting UID for username */
        if (pwd == NULL) {
            purc_set_error (PURC_ERROR_BAD_NAME);
            return PURC_VARIANT_INVALID;
        }

        uid = pwd->pw_uid;
    }

    // If the owner or group is specified as -1, then that ID is not changed.
    if (0 == chown (filename, uid, -1)) {
        ret_var = purc_variant_make_boolean (true);
    }
    else {
        set_purc_error_by_errno ();
        ret_var = purc_variant_make_boolean (false);
    }

    return ret_var;
}

static purc_variant_t
copy_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strncpy (filename, string_filename, sizeof(filename));

    // wait for code

    ret_var = purc_variant_make_boolean (true);
    return ret_var;
}

static purc_variant_t
dirname_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strncpy (filename, string_filename, sizeof(filename));

    // wait for code

    ret_var = purc_variant_make_boolean (true);
    return ret_var;
}

static purc_variant_t
disk_usage_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strncpy (filename, string_filename, sizeof(filename));

    // wait for code

    ret_var = purc_variant_make_boolean (true);
    return ret_var;
}

static purc_variant_t
file_exists_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strncpy (filename, string_filename, sizeof(filename));

    // wait for code

    ret_var = purc_variant_make_boolean (true);
    return ret_var;
}

static purc_variant_t
file_is_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strncpy (filename, string_filename, sizeof(filename));

    // wait for code

    ret_var = purc_variant_make_boolean (true);
    return ret_var;
}

static purc_variant_t
lchgrp_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strncpy (filename, string_filename, sizeof(filename));

    // wait for code

    ret_var = purc_variant_make_boolean (true);
    return ret_var;
}

static purc_variant_t
lchown_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strncpy (filename, string_filename, sizeof(filename));

    // wait for code

    ret_var = purc_variant_make_boolean (true);
    return ret_var;
}

static purc_variant_t
linkinfo_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strncpy (filename, string_filename, sizeof(filename));

    // wait for code

    ret_var = purc_variant_make_boolean (true);
    return ret_var;
}

static purc_variant_t
lstat_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strncpy (filename, string_filename, sizeof(filename));

    // wait for code

    ret_var = purc_variant_make_boolean (true);
    return ret_var;
}

static purc_variant_t
link_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strncpy (filename, string_filename, sizeof(filename));

    // wait for code

    ret_var = purc_variant_make_boolean (true);
    return ret_var;
}

static purc_variant_t
mkdir_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    const char *filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);
    if (NULL == filename) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    if (mkdir (filename, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0)
        ret_var = purc_variant_make_boolean (false);
    else
        ret_var = purc_variant_make_boolean (true);

    return ret_var;
}

static purc_variant_t
pathinfo_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strncpy (filename, string_filename, sizeof(filename));

    // wait for code

    ret_var = purc_variant_make_boolean (true);
    return ret_var;
}

static purc_variant_t
readlink_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strncpy (filename, string_filename, sizeof(filename));

    // wait for code

    ret_var = purc_variant_make_boolean (true);
    return ret_var;
}

static purc_variant_t
realpath_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strncpy (filename, string_filename, sizeof(filename));

    // wait for code

    ret_var = purc_variant_make_boolean (true);
    return ret_var;
}

static purc_variant_t
rename_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strncpy (filename, string_filename, sizeof(filename));

    // wait for code

    ret_var = purc_variant_make_boolean (true);
    return ret_var;
}

static purc_variant_t
rmdir_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    const char *filename = NULL;
    DIR *dirp;
    struct dirent *dp;
    struct stat dir_stat;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    bool empty = true;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);
    if (NULL == filename) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    if (access(filename, F_OK | R_OK) != 0) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
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
stat_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strncpy (filename, string_filename, sizeof(filename));

    // wait for code

    ret_var = purc_variant_make_boolean (true);
    return ret_var;
}

static purc_variant_t
symlink_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strncpy (filename, string_filename, sizeof(filename));

    // wait for code

    ret_var = purc_variant_make_boolean (true);
    return ret_var;
}

static purc_variant_t
tempname_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strncpy (filename, string_filename, sizeof(filename));

    // wait for code

    ret_var = purc_variant_make_boolean (true);
    return ret_var;
}

static purc_variant_t
touch_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    const char *filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);
    if (NULL == filename) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

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
umask_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strncpy (filename, string_filename, sizeof(filename));

    // wait for code

    ret_var = purc_variant_make_boolean (true);
    return ret_var;
}

static purc_variant_t
unlink_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    const char *filename = NULL;
    struct stat filestat;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);
    if (NULL == filename) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    if (access(filename, F_OK | R_OK) != 0) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return purc_variant_make_boolean (false);
    }

    if (stat(filename, &filestat) < 0)
        return purc_variant_make_boolean (false);

    if (S_ISREG(filestat.st_mode)) {
        if (unlink (filename) == 0)
            ret_var = purc_variant_make_boolean (true);
        else
            ret_var = purc_variant_make_boolean (false);
    }
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
rm_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    const char *filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);
    if (NULL == filename) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    if (remove_dir ((char *)filename))
        ret_var = purc_variant_make_boolean (false);
    else
        ret_var = purc_variant_make_boolean (true);

    return ret_var;

}

static purc_variant_t
file_contents_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strncpy (filename, string_filename, sizeof(filename));

    // wait for code

    ret_var = purc_variant_make_boolean (true);
    return ret_var;
}

static purc_variant_t
open_dir_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strncpy (filename, string_filename, sizeof(filename));

    // wait for code

    ret_var = purc_variant_make_boolean (true);
    return ret_var;
}

static purc_variant_t
dir_read_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strncpy (filename, string_filename, sizeof(filename));

    // wait for code

    ret_var = purc_variant_make_boolean (true);
    return ret_var;
}

static purc_variant_t
dir_rewind_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strncpy (filename, string_filename, sizeof(filename));

    // wait for code

    ret_var = purc_variant_make_boolean (true);
    return ret_var;
}

static purc_variant_t pcdvobjs_create_fs(void)
{
    static struct purc_dvobj_method method [] = {
        {"list",          list_getter, NULL},
        {"list_prt",      list_prt_getter, NULL},
        {"basename",      basename_getter, NULL},
        {"chgrp",         chgrp_getter, NULL},
        {"chmod",         chmod_getter, NULL},
        {"chown",         chown_getter, NULL},
        {"copy",          copy_getter, NULL},
        {"dirname",       dirname_getter, NULL},
        {"disk_usage",    disk_usage_getter, NULL},
        {"file_exists",   file_exists_getter, NULL},
        {"file_is",       file_is_getter, NULL},
        {"lchgrp",        lchgrp_getter, NULL},
        {"lchown",        lchown_getter, NULL},
        {"linkinfo",      linkinfo_getter, NULL},
        {"lstat",         lstat_getter, NULL},
        {"link",          link_getter, NULL},
        {"mkdir",         mkdir_getter, NULL},
        {"pathinfo",      pathinfo_getter, NULL},
        {"readlink",      readlink_getter, NULL},
        {"realpath",      realpath_getter, NULL},
        {"rename",        rename_getter, NULL},
        {"rmdir",         rmdir_getter, NULL},
        {"stat",          stat_getter, NULL},
        {"symlink",       symlink_getter, NULL},
        {"tempname",      tempname_getter, NULL},
        {"touch",         touch_getter, NULL},
        {"umask",         umask_getter, NULL},
        {"unlink",        unlink_getter, NULL},
        {"rm",            rm_getter, NULL},// beyond documentation
        {"file_contents", file_contents_getter, NULL},
        {"open_dir",      open_dir_getter, NULL},
        {"dir_read",      dir_read_getter, NULL},
        {"dir_rewind",    dir_rewind_getter, NULL} };

    return purc_dvobj_make_from_methods (method, PCA_TABLESIZE(method));
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
        if (pcutils_strncasecmp (name, dynamic_objects[i].name, strlen (name)) == 0)
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
