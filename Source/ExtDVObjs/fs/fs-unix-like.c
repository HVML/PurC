/*
 * @file fs-posix.c
 * @author LIU Xin, GENG Yue
 * @date 2022/07/31
 * @brief The implementation of file system dynamic variant object for POSIX.
 *
 * Copyright (C) 2022 LIU Xin
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <pwd.h>
#include <errno.h>
#include <stdlib.h>
#include <grp.h>
#include <uuid/uuid.h>

#if OS(LINUX)
#include <mntent.h>
#include <sys/vfs.h>
#endif

#if OS(DARWIN)
#include <sys/param.h>
#include <sys/mount.h>
#endif

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

static inline int strcmp_len (const char *str1, const char *str2, size_t *real_length)
{
    (*real_length) = 0;
    while (*str1 && *str2 && !purc_isspace(*str1) && !purc_isspace(*str2)) {
        if (*str1 != *str2) {
            return (*str1 > *str2) ? 1 : -1;
        }
        
        (*real_length) ++;
        str1 ++;
        str2 ++;
    }
    return 0;
}

static bool filecopy (const char *infile, const char *outfile)
{
    #define FLCPY_BFSZ  8192

    char buffer[FLCPY_BFSZ];
    size_t sz_read = 0;
    FILE *in  = fopen (infile, "rb");
    FILE *out = fopen (outfile,"wb");

    if (NULL == in || NULL == out) {
        if (in)
            fclose (in);
        if (out)
            fclose (out);
        return false;
    }

    while ((sz_read = fread (buffer, 1, FLCPY_BFSZ, in)) == FLCPY_BFSZ) {
        if (sz_read != fwrite (buffer, 1, sz_read, out)) {
            fclose (out);
            fclose (in);
            return false;
        }
    }
    if (sz_read != fwrite (buffer, 1, sz_read, out)) {
        fclose (out);
        fclose (in);
        return false;
    }

    fclose (out);
    fclose (in);
    return true;
}

static void set_purc_error_by_errno (void)
{
    switch (errno) {
        case EACCES:
            purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
            break;

        case ENOENT:
            purc_set_error (PURC_ERROR_ENTITY_NOT_FOUND);
            break;

        case ENOMEM:
            purc_set_error (PURC_ERROR_OUT_OF_MEMORY);
            break;

        case ENAMETOOLONG:
            purc_set_error (PURC_ERROR_TOO_LONG);
            break;

        case EIO:
            purc_set_error (PURC_ERROR_SYS_FAULT);
            break;

        default:
            purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
    }
}

// Transmit mode string to mode_t
#define INVALID_MODE       (mode_t)-1
#define SET_USER_ID        0x01
#define SET_GROUP_ID       0x02
#define SET_OTHER_ID       0x04
#define STAGE_NOT_SET      0
#define STAGE_TARGET       1
#define STAGE_OPERATOR     2
#define STAGE_NEED_VALUE   3
#define STAGE_VALUE        4
static inline void set_mode_value (mode_t *ptr_mode, unsigned int op_target,
        char operator, unsigned int op_value)
{
    unsigned int op_value_mask = 0;

    if (op_target | SET_USER_ID) {
        op_value_mask |= (op_value << 6);
    }
    if (op_target | SET_GROUP_ID) {
        op_value_mask |= (op_value << 3);
    }
    if (op_target | SET_OTHER_ID) {
        op_value_mask |= op_value;
    }

    switch (operator) {
        case '=':
            *ptr_mode = op_value_mask;
            break;

        case '+':
            *ptr_mode |= op_value_mask;
            break;

        case '-':
            *ptr_mode &= (~ op_value_mask);
            break;
    }
}

static mode_t str_to_mode (const char *input, mode_t mode)
{
    unsigned int op_target = 0;
    unsigned int op_value = 0;
    int operator = '\0'; /* =, -, + */
    int op_stage = STAGE_TARGET;

    while (*input) {

        switch (op_stage) {
        case STAGE_NOT_SET:
            op_stage = STAGE_TARGET;
            break;

        case STAGE_TARGET: {
            switch (*input) {
            case 'u':
                op_target |= SET_USER_ID;
                input ++;
                break;

            case 'g':
                op_target |= SET_GROUP_ID;
                input ++;
                break;

            case 'o':
                op_target |= SET_OTHER_ID;
                input ++;
                break;

            case 'a':
                op_target = (SET_USER_ID | SET_GROUP_ID | SET_OTHER_ID);
                input ++;
                break;

            case ',':
                input ++; // Skip redundant commas.
                break;

            default:
                op_stage = STAGE_OPERATOR;
            }
            break;
        }

        case STAGE_OPERATOR: {
            switch (*input) {
                case '+':
                case '-':
                case '=':
                    operator = *input;
                    op_stage = STAGE_NEED_VALUE;
                    input ++;
                    break;

                default:
                    return INVALID_MODE;
            }
            break;
        }

        case STAGE_NEED_VALUE:
        case STAGE_VALUE: {
            switch (*input) {
                case 'r':
                    op_stage = STAGE_VALUE;
                    op_value |= 0x04;
                    input ++;
                    break;

                case 'w':
                    op_stage = STAGE_VALUE;
                    op_value |= 0x02;
                    input ++;
                    break;

                case 'x':
                    op_stage = STAGE_VALUE;
                    op_value |= 0x01;
                    input ++;
                    break;

                case ',':
                    set_mode_value (&mode, op_target, operator, op_value);
                    op_stage = STAGE_NOT_SET;
                    input ++;
                    break;

                default:
                    return INVALID_MODE; // Error occurred
            }
            break;
        }

        }
    }

    if (STAGE_VALUE == op_stage) {
        set_mode_value (&mode, op_target, operator, op_value);
        return mode; // Normal return
    }

    if (STAGE_NOT_SET == op_stage) {
        return mode; // Normal return
    }

    return INVALID_MODE; // Incomplete statement
}

static const char *get_basename (const char *string_path,
        const char *string_suffix, size_t *length)
{
    // On Linux, slash (/) is used as directory separator character.
    const char separator = '/';
    const char *base_begin = string_path;
    const char *temp_ptr = string_path;
    const char *base_end = string_path + strlen (string_path);

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

    (*length) = (base_end - base_begin);
    return base_begin;
}

static const char *get_basename_ex (const char *string_path,
        const char **ext_begin,
        size_t *base_length,
        size_t *fname_length,
        size_t *ext_length)
{
    // On Linux, slash (/) is used as directory separator character.
    const char separator = '/';
    const char *base_begin = string_path;
    const char *temp_ptr = string_path;
    const char *base_end = string_path + strlen (string_path);

    while (temp_ptr < base_end) {
        if (separator == *temp_ptr) {
            base_begin = temp_ptr + 1;
        }
        temp_ptr ++;
    }
    (*fname_length) = (base_end - base_begin);

    temp_ptr = base_begin;
    while (temp_ptr < base_end) {
        if ('.' == *temp_ptr) {
            (*ext_begin) = temp_ptr + 1;
            (*fname_length) = (temp_ptr - base_begin);
        }
        temp_ptr ++;
    }

    (*base_length) = (base_end - base_begin);
    if ((*ext_begin) != NULL) {
        (*ext_length) = (base_end - (*ext_begin));
    }
    return base_begin;
}

static const char *get_dir_path (const char *string_path,
        uint64_t levels, size_t *length)
{
    // On Linux, slash (/) is used as directory separator character.
    const char separator = '/';
    const char *temp_ptr = NULL;
    const char *dir_begin = string_path;
    const char *dir_end = dir_begin + strlen(string_path) - 1;

    while (separator != *dir_begin && '\0' != *dir_begin) {
        dir_begin ++;
    }

    while (levels --) {
        temp_ptr = dir_end;
        while (temp_ptr >= dir_begin && separator == *temp_ptr) {
            temp_ptr--;
        }
        while (temp_ptr >= dir_begin && separator != *temp_ptr) {
            temp_ptr--;
        }
        if (temp_ptr <= dir_begin) {
            if (separator == *dir_begin) {
                dir_end = dir_begin + 1;
            }
            else {
                dir_end = dir_begin;
            }
            break;
        }
        dir_end = temp_ptr;
    }

    (*length) = (dir_end - string_path);
    return string_path;
}

#if OS(LINUX)
static bool find_mountpoint (char *dir)
{
    // On Linux, slash (/) is used as directory separator character.
    const char separator = '/';
    char  *dir_end = dir + strlen(dir) - 1;
    struct stat  st;
    dev_t  orig_dev;
    char   last_char;
    char  *last_char_pos;

    if (stat (dir, &st) != 0)
        return false;
    
    orig_dev = st.st_dev;

    while (dir_end > (dir + 1)) {

        while (dir_end > dir && separator == *dir_end) {
            dir_end--;
        }
        while (dir_end > dir && separator != *dir_end) {
            dir_end--;
        }
        if (dir_end <= dir) {
            last_char_pos = dir + 1;
        }
        else {
            last_char_pos = dir_end;
            (*dir_end) = '\0';
        }

        // set dir to dirname
        last_char = (*last_char_pos);
        (*last_char_pos) = '\0';

        // printf ("------ find_mountpoint: current path is: %s\n", dir);
        if (stat (dir, &st) != 0) {
            // printf ("------ find_mountpoint: get st error\n");
            return false;
        }

        // printf ("------ find_mountpoint: current st_dev is: %d\n", (int)st.st_dev);
        
        if (st.st_dev != orig_dev) {// we crossed the device border
            // printf ("------ find_mountpoint: normal return\n");
            (*last_char_pos) = last_char;
            return true;
        }
    }

    // printf ("------ find_mountpoint: search to '/'\n");
    return true;
}
#endif


enum {
    FN_OPTION_STAT,
    FN_OPTION_LSTAT,
};
static purc_variant_t
get_stat_result (int nr_fn_option, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    const char *string_filename = NULL;
    const char *string_flags = "type mode_digits uid gid size rdev ctime";
    const char *flag = NULL;
    struct stat st;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (NULL == string_filename) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (nr_args > 1) {
        string_flags = purc_variant_get_string_const (argv[1]);
        if (NULL == string_flags) {
            purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        if (strcmp(string_flags, "all") == 0) {
            string_flags = "dev inode type mode_digits mode_alphas nlink \
                    uid gid size rdev blksize blocks atime ctime mtime";
        }
        else if (strcmp(string_flags, "default") == 0) {
            ; // Nothing to do
        }
    }

    switch (nr_fn_option)
    {
        case FN_OPTION_STAT:
            if (lstat(string_filename, &st) == -1) {
                purc_set_error (PURC_ERROR_WRONG_STAGE);
                goto failed;
            }
            break;

        case FN_OPTION_LSTAT:
            if (lstat(string_filename, &st) == -1) {
                purc_set_error (PURC_ERROR_WRONG_STAGE);
                goto failed;
            }
            break;

        default:
            purc_set_error (PURC_ERROR_INTERNAL_FAILURE);
            goto failed;
    }

    ret_var = purc_variant_make_object (0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);

    flag = string_flags;
    while (*flag)
    {
        size_t flag_len = 0;

        while (purc_isspace(*flag))
            flag ++;

        switch (*flag) {
        case 'd':
            if (strcmp_len (flag, "dev", &flag_len) == 0) {
                // returns ID of device containing the file.
                // dev_major
                val = purc_variant_make_ulongint ((long) major(st.st_dev));
                purc_variant_object_set_by_static_ckey (ret_var, "dev_major", val);
                purc_variant_unref (val);

                // dev_minor
                val = purc_variant_make_ulongint ((long) major(st.st_dev));
                purc_variant_object_set_by_static_ckey (ret_var, "dev_minor", val);
                purc_variant_unref (val);
            }
            break;

        case 'i':
            if (strcmp_len (flag, "inode", &flag_len) == 0) {
                // returns inode number.
                val = purc_variant_make_ulongint (st.st_ino);
                purc_variant_object_set_by_static_ckey (ret_var, "inode", val);
                purc_variant_unref (val);
            }
            break;

        case 't':
            if (strcmp_len (flag, "type", &flag_len) == 0) {
                // returns file type like 'd', 'b', or 's'.

                const char *string_type = NULL;
                switch (st.st_mode & S_IFMT) {
                    case S_IFBLK:  string_type = "block device";        break;
                    case S_IFCHR:  string_type = "character device";    break;
                    case S_IFDIR:  string_type = "directory";           break;
                    case S_IFIFO:  string_type = "FIFO/pipe";           break;
                    case S_IFLNK:  string_type = "symlink";             break;
                    case S_IFREG:  string_type = "regular file";        break;
                    case S_IFSOCK: string_type = "socket";              break;
                    default:       string_type = "unknown";             break;
                }

                val = purc_variant_make_string (string_type, true);
                purc_variant_object_set_by_static_ckey (ret_var, "type", val);
                purc_variant_unref (val);
            }
            break;

        case 'm':
            if (strcmp_len (flag, "mode_digits", &flag_len) == 0) {
                // returns file mode like '0644'.
                char string_mode[] = "0000";
                string_mode[1] += (st.st_mode & 0x01C0) >> 6;
                string_mode[2] += (st.st_mode & 0x0038) >> 3;
                string_mode[3] += (st.st_mode & 0x0007);
                val = purc_variant_make_string (string_mode, true);
                purc_variant_object_set_by_static_ckey (ret_var, "type", val);
                purc_variant_unref (val);
            }
            else if (strcmp_len (flag, "mode_alphas", &flag_len) == 0) {
                // returns file mode like 'rwxrwxr-x'.
                char string_mode[] = "---------";
                if (st.st_mode & S_IRUSR) string_mode[0] = 'r';
                if (st.st_mode & S_IWUSR) string_mode[1] = 'w';
                if (st.st_mode & S_IXUSR) string_mode[2] = 'x';
                if (st.st_mode & S_IRGRP) string_mode[3] = 'r';
                if (st.st_mode & S_IWGRP) string_mode[4] = 'w';
                if (st.st_mode & S_IXGRP) string_mode[5] = 'x';
                if (st.st_mode & S_IROTH) string_mode[6] = 'r';
                if (st.st_mode & S_IWOTH) string_mode[7] = 'w';
                if (st.st_mode & S_IXOTH) string_mode[8] = 'x';
                val = purc_variant_make_string (string_mode, true);
                purc_variant_object_set_by_static_ckey (ret_var, "type", val);
                purc_variant_unref (val);
            }
            else if (strcmp_len (flag, "mtime", &flag_len) == 0) {
                // returns time of last modification.
#if OS(LINUX)
                val = purc_variant_make_ulongint (st.st_mtim.tv_sec);
#elif OS(DARWIN)
                val = purc_variant_make_ulongint (st.st_mtimespec.tv_sec);
#else
#error "Unknown Operating System"
#endif
                purc_variant_object_set_by_static_ckey (ret_var, "mtime_sec", val);
                purc_variant_unref (val);

#if OS(LINUX)
                val = purc_variant_make_ulongint (st.st_mtim.tv_nsec);
#elif OS(DARWIN)
                val = purc_variant_make_ulongint (st.st_mtimespec.tv_nsec);
#else
#error "Unknown Operating System"
#endif
                purc_variant_object_set_by_static_ckey (ret_var, "mtime_nsec", val);
                purc_variant_unref (val);
            }
            break;

        case 'n':
            if (strcmp_len (flag, "nlink", &flag_len) == 0) {
                // returns number of hard links.
                val = purc_variant_make_number (st.st_nlink);
                purc_variant_object_set_by_static_ckey (ret_var, "nlink", val);
                purc_variant_unref (val);
            }
            break;

        case 'u':
            if (strcmp_len (flag, "uid", &flag_len) == 0) {
                // returns the user ID of owner.
                val = purc_variant_make_number (st.st_uid);
                purc_variant_object_set_by_static_ckey (ret_var, "uid", val);
                purc_variant_unref (val);
            }
            break;

        case 'g':
            if (strcmp_len (flag, "gid", &flag_len) == 0) {
                // returns the group ID of owner.
                val = purc_variant_make_number (st.st_gid);
                purc_variant_object_set_by_static_ckey (ret_var, "gid", val);
                purc_variant_unref (val);
            }
            break;

        case 'r':
            if (strcmp_len (flag, "rdev", &flag_len) == 0) {
                // returns the device ID if it is a special file.
                // dev_major
                val = purc_variant_make_ulongint ((long) major(st.st_rdev));
                purc_variant_object_set_by_static_ckey (ret_var, "rdev_major", val);
                purc_variant_unref (val);

                // dev_minor
                val = purc_variant_make_ulongint ((long) major(st.st_rdev));
                purc_variant_object_set_by_static_ckey (ret_var, "rdev_minor", val);
                purc_variant_unref (val);
            }
            break;

        case 's':
            if (strcmp_len (flag, "size", &flag_len) == 0) {
                // returns total size in bytes.
                val = purc_variant_make_ulongint (st.st_size);
                purc_variant_object_set_by_static_ckey (ret_var, "size", val);
                purc_variant_unref (val);
            }
            break;

        case 'b':
            if (strcmp_len (flag, "blksize", &flag_len) == 0) {
                // returns block size for filesystem I/O.
                val = purc_variant_make_ulongint (st.st_blksize);
                purc_variant_object_set_by_static_ckey (ret_var, "blksize", val);
                purc_variant_unref (val);
            }
            else if (strcmp_len (flag, "blocks", &flag_len) == 0) {
                // returns number of 512B blocks allocated.
                val = purc_variant_make_ulongint (st.st_blocks);
                purc_variant_object_set_by_static_ckey (ret_var, "blocks", val);
                purc_variant_unref (val);
            }
            break;

        case 'a':
            if (strcmp_len (flag, "atime", &flag_len) == 0) {
                // returns time of last acces.
#if OS(LINUX)
                val = purc_variant_make_ulongint (st.st_atim.tv_sec);
#elif OS(DARWIN)
                val = purc_variant_make_ulongint (st.st_atimespec.tv_sec);
#else
#error "Unknown Operating System"
#endif
                purc_variant_object_set_by_static_ckey (ret_var, "atime_sec", val);
                purc_variant_unref (val);

#if OS(LINUX)
                val = purc_variant_make_ulongint (st.st_atim.tv_nsec);
#elif OS(DARWIN)
                val = purc_variant_make_ulongint (st.st_atimespec.tv_nsec);
#else
#error "Unknown Operating System"
#endif
                purc_variant_object_set_by_static_ckey (ret_var, "atime_nsec", val);
                purc_variant_unref (val);
            }
            break;

        case 'c':
            if (strcmp_len (flag, "ctime", &flag_len) == 0) {
                // returns time of last status change.
#if OS(LINUX)
                val = purc_variant_make_ulongint (st.st_ctim.tv_sec);
#elif OS(DARWIN)
                val = purc_variant_make_ulongint (st.st_ctimespec.tv_sec);
#else
#error "Unknown Operating System"
#endif
                purc_variant_object_set_by_static_ckey (ret_var, "ctime_sec", val);
                purc_variant_unref (val);

#if OS(LINUX)
                val = purc_variant_make_ulongint (st.st_ctim.tv_nsec);
#elif OS(DARWIN)
                val = purc_variant_make_ulongint (st.st_ctimespec.tv_nsec);
#else
#error "Unknown Operating System"
#endif
                purc_variant_object_set_by_static_ckey (ret_var, "ctime_nsec", val);
                purc_variant_unref (val);
            }
            break;

        default:
            purc_variant_unref (ret_var);
            purc_set_error (PURC_ERROR_WRONG_STAGE);
            goto failed;
        }

        if (0 == flag_len)
            break;

        flag += flag_len;
    }

    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);;

    return PURC_VARIANT_INVALID;
}


static purc_variant_t
list_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    char dir_name[PATH_MAX + 1];
    char filename[PATH_MAX + NAME_MAX + 1];
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
        goto failed;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (NULL == string_filename) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }
    strncpy (dir_name, string_filename, sizeof(dir_name)-1);

    if (access(dir_name, F_OK | R_OK) != 0) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        goto failed;
    }

    // get the filter
    if ((nr_args > 1) && (argv[1] == NULL ||
            (!purc_variant_is_string (argv[1])))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
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
                if (wildcard == NULL) {
                    purc_set_error (PURC_ERROR_OUT_OF_MEMORY);
                    goto discontinue;
                }

                temp_wildcard = wildcard;
            }
            else {
                temp_wildcard->next = malloc (sizeof(struct wildcard_list));
                if (temp_wildcard->next == NULL) {
                    purc_set_error (PURC_ERROR_OUT_OF_MEMORY);
                    goto discontinue;
                }

                temp_wildcard = temp_wildcard->next;
            }
            temp_wildcard->next = NULL;
            temp_wildcard->wildcard = malloc (length + 1);
            if (temp_wildcard->wildcard == NULL) {
                purc_set_error (PURC_ERROR_OUT_OF_MEMORY);
                goto discontinue;
            }

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
        goto discontinue;
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

        strncpy (filename, dir_name, sizeof(filename)-1);
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
        val = purc_variant_make_ulongint(file_stat.st_atime);
        purc_variant_object_set_by_static_ckey (obj_var, "atime_sec", val);
        purc_variant_unref (val);

#if OS(LINUX)
        val = purc_variant_make_ulongint (file_stat.st_atim.tv_nsec);
#elif OS(DARWIN)
        val = purc_variant_make_ulongint (file_stat.st_atimespec.tv_nsec);
#endif
        purc_variant_object_set_by_static_ckey (obj_var, "atime_nsec", val);
        purc_variant_unref (val);

        // mtime
        val = purc_variant_make_ulongint(file_stat.st_mtime);
        purc_variant_object_set_by_static_ckey (obj_var, "mtime_sec", val);
        purc_variant_unref (val);

#if OS(LINUX)
        val = purc_variant_make_ulongint (file_stat.st_mtim.tv_nsec);
#elif OS(DARWIN)
        val = purc_variant_make_ulongint (file_stat.st_mtimespec.tv_nsec);
#endif
        purc_variant_object_set_by_static_ckey (obj_var, "mtime_nsec", val);
        purc_variant_unref (val);

        // ctime
        val = purc_variant_make_ulongint(file_stat.st_ctime);
        purc_variant_object_set_by_static_ckey (obj_var, "ctime_sec", val);
        purc_variant_unref (val);

#if OS(LINUX)
        val = purc_variant_make_ulongint (file_stat.st_ctim.tv_nsec);
#elif OS(DARWIN)
        val = purc_variant_make_ulongint (file_stat.st_ctimespec.tv_nsec);
#endif
        purc_variant_object_set_by_static_ckey (obj_var, "ctime_nsec", val);
        purc_variant_unref (val);

        purc_variant_array_append (ret_var, obj_var);
        purc_variant_unref (obj_var);
    }

    closedir(dir);

discontinue:
    while (wildcard) {
        if (wildcard->wildcard)
            free (wildcard->wildcard);
        temp_wildcard = wildcard;
        wildcard = wildcard->next;
        free (temp_wildcard);
    }
    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
list_prt_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
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
    char dir_name[PATH_MAX + 1];
    char filename[PATH_MAX + NAME_MAX + 1];
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
        goto failed;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (NULL == string_filename) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }
    strncpy (dir_name, string_filename, sizeof(dir_name)-1);

    if (access(dir_name, F_OK | R_OK) != 0) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        goto failed;
    }

    // get the filter
    if ((nr_args > 1) && (argv[1] != PURC_VARIANT_INVALID &&
            (!purc_variant_is_string (argv[1])))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
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
                if (wildcard == NULL) {
                    purc_set_error (PURC_ERROR_OUT_OF_MEMORY);
                    goto discontinue;
                }

                temp_wildcard = wildcard;
            }
            else {
                temp_wildcard->next = malloc (sizeof(struct wildcard_list));
                if (temp_wildcard->next == NULL) {
                    purc_set_error (PURC_ERROR_OUT_OF_MEMORY);
                    goto discontinue;
                }

                temp_wildcard = temp_wildcard->next;
            }
            temp_wildcard->next = NULL;
            temp_wildcard->wildcard = malloc (length + 1);
            if (temp_wildcard->wildcard == NULL) {
                purc_set_error (PURC_ERROR_OUT_OF_MEMORY);
                goto discontinue;
            }

            strncpy(temp_wildcard->wildcard, head, length);
            *(temp_wildcard->wildcard + length) = 0x00;
            pcdvobjs_remove_space (temp_wildcard->wildcard);
            head = pcutils_get_next_token (head + length + 1, ";", &length);
        }
    }

    // get the mode
    if ((nr_args > 2) && (argv[2] == NULL || (!purc_variant_is_string (argv[2])))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto discontinue;
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
        set_purc_error_by_errno();
        goto discontinue;
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

        strncpy (filename, dir_name, sizeof(filename)-1);
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

discontinue:
    while (wildcard) {
        if (wildcard->wildcard)
            free (wildcard->wildcard);
        temp_wildcard = wildcard;
        wildcard = wildcard->next;
        free (temp_wildcard);
    }
    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
basename_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *string_path = NULL;
    const char *string_suffix = NULL;
    const char *base_begin;
    size_t length;
    purc_variant_t ret_string = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the parameters
    string_path = purc_variant_get_string_const (argv[0]);
    if (NULL == string_path) {
        ret_string = purc_variant_make_string("", true);
        return ret_string;
    }
    if (nr_args > 1) {
        string_suffix = purc_variant_get_string_const (argv[1]);
        if (NULL == string_suffix) {
            purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }
    }

    base_begin = get_basename (string_path, string_suffix, &length);
    ret_string = purc_variant_make_string_ex(base_begin, length, true);
    return ret_string;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
chgrp_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *filename = NULL;
    const char *string_group = NULL;
    uint64_t uint_gid; 
    gid_t gid;
    char *endptr;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 2) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // Get the file name
    filename = purc_variant_get_string_const (argv[0]);

    // Get the group name (or gid)
    if (purc_variant_cast_to_ulongint (argv[1], &uint_gid, false)) {
        gid = (gid_t)uint_gid;
    }
    else {
        string_group = purc_variant_get_string_const (argv[1]);
        if (NULL == filename || NULL == string_group) {
            purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        // group : string_name | gid
        gid = strtol(string_group, &endptr, 10);  /* Allow a numeric string */

        if (*endptr != '\0') {              /* Was not pure numeric string */
            struct group *grp = getgrnam(string_group);
            if (grp == NULL) {
                purc_set_error (PURC_ERROR_BAD_NAME);
                return PURC_VARIANT_INVALID;
            }

            gid = grp->gr_gid;
        }
    }

    // If the group_id is specified as -1, then that ID is not changed.
    if (0 == chown (filename, -1, gid)) {
        ret_var = purc_variant_make_boolean (true);
    }
    else {
        set_purc_error_by_errno ();
        ret_var = purc_variant_make_boolean (false);
    }

    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
chmod_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *filename = NULL;
    const char *string_mode = NULL;
    mode_t new_mode;
    char *endptr;
    struct stat filestat;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 2) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the file name and the mode
    filename = purc_variant_get_string_const (argv[0]);
    string_mode = purc_variant_get_string_const (argv[1]);
    if (NULL == filename || NULL == string_mode) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    // new_mode : string_mode | int (octal | decimal)
    if ('0' == string_mode[0]) {
        new_mode = strtol (string_mode, &endptr, 8);  /* Octal number */
    }
    else {
        new_mode = strtol (string_mode, &endptr, 10); /* Decimal */
    }

    if (*endptr != '\0') {              /* Was not pure numeric string */

        if (stat(filename, &filestat) < 0) {
            set_purc_error_by_errno ();
            goto failed;
        }

        new_mode = str_to_mode (string_mode, 0xFFF & filestat.st_mode);
        if (INVALID_MODE == new_mode) {
            purc_set_error (PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
    }

    if (0 == chmod (filename, new_mode)) {
        ret_var = purc_variant_make_boolean (true);
    }
    else {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        goto failed;
    }

    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
chown_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *filename = NULL;
    const char *string_owner = NULL;
    uint64_t uint_uid;
    uid_t uid;
    struct passwd *pwd;
    char *endptr;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 2) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // Get the file name
    filename = purc_variant_get_string_const (argv[0]);

    // Get the user name (or uid)
    if (purc_variant_cast_to_ulongint (argv[1], &uint_uid, false)) {
        uid = (gid_t)uint_uid;
    }
    else {
        string_owner = purc_variant_get_string_const (argv[1]);
        if (NULL == filename || NULL == string_owner) {
            purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        // owner : string_name | uid
        uid = strtol (string_owner, &endptr, 10);  /* Allow a numeric string */

        if (*endptr != '\0') {               /* Was not pure numeric string */
            pwd = getpwnam (string_owner);   /* Try getting UID for username */
            if (pwd == NULL) {
                set_purc_error_by_errno ();
                goto failed;
            }

            uid = pwd->pw_uid;
        }
    }

    // If the owner_id is specified as -1, then that ID is not changed.
    if (0 == chown (filename, uid, -1)) {
        ret_var = purc_variant_make_boolean (true);
    }
    else {
        set_purc_error_by_errno ();
        goto failed;
    }

    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
copy_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *filename_from = NULL;
    const char *filename_to = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 2) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the file name
    filename_from = purc_variant_get_string_const (argv[0]);
    filename_to   = purc_variant_get_string_const (argv[1]);
    if (NULL == filename_from || NULL == filename_to) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (filecopy (filename_from, filename_to)) {
        ret_var = purc_variant_make_boolean (true);
    }
    else {
        set_purc_error_by_errno ();
        goto failed;
    }

    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
dirname_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *string_path = NULL;
    const char *dir_begin = NULL;
    uint64_t levels = 1;
    size_t length;
    purc_variant_t ret_string = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the parameters
    string_path = purc_variant_get_string_const (argv[0]);
    if (NULL == string_path) {
        ret_string = purc_variant_make_string("", true);
        return ret_string;
    }
    if (nr_args > 1) {
        // Get the levels
        if (! purc_variant_cast_to_ulongint (argv[1], &levels, false)) {
            purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }
    }

    dir_begin = get_dir_path (string_path, levels, &length);
    ret_string = purc_variant_make_string_ex(dir_begin, length, true);
    return ret_string;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
disk_usage_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *string_dir = NULL;
#if OS(LINUX)
    char mntpoint_buffer[PATH_MAX + 1];
#endif
    struct statfs fsu;
    struct stat   st;
    purc_variant_t val = PURC_VARIANT_INVALID;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the directory
    string_dir = purc_variant_get_string_const (argv[0]);
    if (NULL == string_dir) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (statfs (string_dir, &fsu) != 0) {
        set_purc_error_by_errno ();
        goto failed;
    }

    if (stat (string_dir, &st) != 0) {
        set_purc_error_by_errno ();
        goto failed;
    }

    ret_var = purc_variant_make_object_0 ();

    // free_blocks
    val = purc_variant_make_ulongint (fsu.f_bfree);
    purc_variant_object_set_by_static_ckey (ret_var, "free_blocks", val);
    purc_variant_unref (val);

    // free_inodes
    val = purc_variant_make_ulongint (fsu.f_ffree);
    purc_variant_object_set_by_static_ckey (ret_var, "free_inodes", val);
    purc_variant_unref (val);

    // total_blocks
    val = purc_variant_make_ulongint (fsu.f_blocks);
    purc_variant_object_set_by_static_ckey (ret_var, "total_blocks", val);
    purc_variant_unref (val);

    // total_inodes
    val = purc_variant_make_ulongint (fsu.f_files);
    purc_variant_object_set_by_static_ckey (ret_var, "total_inodes", val);
    purc_variant_unref (val);

    // mount_point
#if OS(LINUX)
    strncpy (mntpoint_buffer, string_dir, sizeof(mntpoint_buffer)-1);
    mntpoint_buffer[sizeof(mntpoint_buffer)-1] = '\0';
    if (find_mountpoint (mntpoint_buffer)) {
        val = purc_variant_make_string(mntpoint_buffer, false);
    }
    else {
        val = purc_variant_make_string("/", false);
    }
#elif OS(DARWIN)
    val = purc_variant_make_string (fsu.f_mntonname, false);
#endif
    purc_variant_object_set_by_static_ckey (ret_var, "mount_point", val);
    purc_variant_unref (val);

    // dev_major
    val = purc_variant_make_ulongint ((long) major(st.st_dev));
    purc_variant_object_set_by_static_ckey (ret_var, "dev_major", val);
    purc_variant_unref (val);

    // dev_minor
    val = purc_variant_make_ulongint ((long) minor(st.st_dev));
    purc_variant_object_set_by_static_ckey (ret_var, "dev_minor", val);
    purc_variant_unref (val);

    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
file_exists_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the file name or dir name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (NULL == string_filename) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (access(string_filename, F_OK | R_OK) == 0)  {
        ret_var = purc_variant_make_boolean (true);
    }
    else {
        ret_var = purc_variant_make_boolean (false);
    }

    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
file_is_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *string_filename = NULL;
    const char *string_which = NULL;
    struct stat st;

    if (nr_args < 2) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the file name and which type
    string_filename = purc_variant_get_string_const (argv[0]);
    string_which = purc_variant_get_string_const (argv[1]);

    if (NULL == string_filename || NULL == string_which) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (lstat(string_filename, &st) == -1) {
        set_purc_error_by_errno ();
        goto failed;
    }

    switch (string_which[0]) {
        case 'd':
            if (strcmp(string_which, "dir") == 0) {
                if (S_IFDIR == (st.st_mode & S_IFMT)) {
                    goto success;
                }
            }
            break;

        case 'f':
            if (strcmp(string_which, "file") == 0) {
                if (S_IFREG == (st.st_mode & S_IFMT)) {
                    goto success;
                }
            }
            break;

        case 's':
            if (strcmp(string_which, "symlink") == 0) {
                if (S_IFLNK == (st.st_mode & S_IFMT)) {
                    goto success;
                }
            }
            if (strcmp(string_which, "socket") == 0) {
                if (S_IFSOCK == (st.st_mode & S_IFMT)) {
                    goto success;
                }
            }
            break;

        case 'p':
            if (strcmp(string_which, "pipe") == 0) {
                if (S_IFIFO == (st.st_mode & S_IFMT)) {
                    goto success;
                }
            }
            break;

        case 'b':
            if (strcmp(string_which, "block") == 0) {
                if (S_IFBLK == (st.st_mode & S_IFMT)) {
                    goto success;
                }
            }
            break;

        case 'c':
            if (strcmp(string_which, "char") == 0) {
                if (S_IFCHR == (st.st_mode & S_IFMT)) {
                    goto success;
                }
            }
            break;

        case 'e':
            if (strcmp(string_which, "exe") == 0 ||
                strcmp(string_which, "executable") == 0) {
                if (0 == access(string_filename, X_OK)) {
                    goto success;
                }
            }
            break;

        case 'r':
            if (strcmp(string_which, "read") == 0 ||
                strcmp(string_which, "readable") == 0) {
                if (0 == access(string_filename, R_OK)) {
                    goto success;
                }
            }
            break;

        case 'w':
            if (strcmp(string_which, "write") == 0 ||
                strcmp(string_which, "writable") == 0) {
                if (0 == access(string_filename, W_OK)) {
                    goto success;
                }
            }
            break;
    }

    return purc_variant_make_boolean (false);

success:
    return purc_variant_make_boolean (true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
lchgrp_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *filename = NULL;
    const char *string_group = NULL;
    uint64_t uint_gid; 
    gid_t gid;
    char *endptr;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 2) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // Get the file name
    filename = purc_variant_get_string_const (argv[0]);

    // Get the group name (or gid)
    if (purc_variant_cast_to_ulongint (argv[1], &uint_gid, false)) {
        gid = (gid_t)uint_gid;
    }
    else {
        string_group = purc_variant_get_string_const (argv[1]);
        if (NULL == filename || NULL == string_group) {
            purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        // group : string_name | gid
        gid = strtol(string_group, &endptr, 10);  /* Allow a numeric string */

        if (*endptr != '\0') {              /* Was not pure numeric string */
            struct group *grp = getgrnam(string_group);
            if (grp == NULL) {
                purc_set_error (PURC_ERROR_BAD_NAME);
                goto failed;
            }

            gid = grp->gr_gid;
        }
    }

    // If the group_id is specified as -1, then that ID is not changed.
    if (0 == lchown (filename, -1, gid)) {
        ret_var = purc_variant_make_boolean (true);
    }
    else {
        set_purc_error_by_errno ();
        goto failed;
    }

    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
lchown_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *filename = NULL;
    const char *string_owner = NULL;
    uint64_t uint_uid;
    uid_t uid;
    struct passwd *pwd;
    char *endptr;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 2) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // Get the file name
    filename = purc_variant_get_string_const (argv[0]);

    // Get the user name (or uid)
    if (purc_variant_cast_to_ulongint (argv[1], &uint_uid, false)) {
        uid = (gid_t)uint_uid;
    }
    else {
        string_owner = purc_variant_get_string_const (argv[1]);
        if (NULL == filename || NULL == string_owner) {
            purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        // owner : string_name | uid
        uid = strtol (string_owner, &endptr, 10);  /* Allow a numeric string */

        if (*endptr != '\0') {               /* Was not pure numeric string */
            pwd = getpwnam (string_owner);   /* Try getting UID for username */
            if (pwd == NULL) {
                purc_set_error (PURC_ERROR_BAD_NAME);
                goto failed;
            }

            uid = pwd->pw_uid;
        }
    }

    // If the owner_id is specified as -1, then that ID is not changed.
    if (0 == lchown (filename, uid, -1)) {
        ret_var = purc_variant_make_boolean (true);
    }
    else {
        set_purc_error_by_errno ();
        goto failed;
    }

    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
linkinfo_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *string_filename = NULL;
    struct stat st;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (NULL == string_filename) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (lstat(string_filename, &st) == -1) {
        set_purc_error_by_errno ();
        goto failed;
    }

    ret_var = purc_variant_make_number (st.st_dev);
    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
lstat_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    return get_stat_result (FN_OPTION_LSTAT, nr_args, argv, call_flags);
}

static purc_variant_t
link_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *string_target = NULL;
    const char *string_link = NULL;

    if (nr_args < 2) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the parameters
    string_target = purc_variant_get_string_const (argv[0]);
    string_link = purc_variant_get_string_const (argv[1]);
    if (NULL == string_target || NULL == string_link) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (link(string_target, string_link) == -1) {
        set_purc_error_by_errno ();
        goto failed;
    }

    return purc_variant_make_boolean (true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
mkdir_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *filename = NULL;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);
    if (NULL == filename) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (mkdir (filename, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
        return purc_variant_make_boolean (true);
    }

    set_purc_error_by_errno ();

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
pathinfo_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *string_path = NULL;
    const char *string_flags = "dirname basename extension filename";
    const char *dir_begin = NULL;
    const char *base_begin = NULL;
    const char *ext_begin = NULL;
    const char *flag = NULL;
    size_t dir_length = 0;
    size_t base_length = 0;
    size_t fname_length = 0;
    size_t ext_length = 0;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the parameters
    string_path = purc_variant_get_string_const (argv[0]);
    if (NULL == string_path) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (nr_args > 1) {
        const char *string_param = NULL;
        string_param = purc_variant_get_string_const (argv[1]);
        if (NULL == string_flags) {
            purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        if (strcmp(string_param, "all") == 0) {
            ; // Nothing to do
        }
        else {
            string_flags = string_param;
        }
    }

    ret_var = purc_variant_make_object (0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);

    flag = string_flags;
    while (*flag) {
        size_t flag_len = 0;

        while (purc_isspace(*flag))
            flag ++;

        switch (*flag) {
            case 'd':
                if (strcmp_len (flag, "dirname", &flag_len) == 0) {
                    dir_begin = get_dir_path (string_path, 1, &dir_length);
                    val = dir_begin ?
                            purc_variant_make_string_ex(dir_begin, dir_length, true)
                          : purc_variant_make_null();
                    purc_variant_object_set_by_static_ckey (ret_var, "dirname", val);
                    purc_variant_unref (val);
                }
                break;

            case 'b':
            case 'e':
            case 'f':
                if (NULL == base_begin) {
                    base_begin = get_basename_ex (string_path, &ext_begin,
                            &base_length, &fname_length, &ext_length);
                }

                if (strcmp_len (flag, "basename", &flag_len) == 0) {
                    val = base_begin ?
                            purc_variant_make_string_ex (base_begin, base_length, true)
                          : purc_variant_make_null();
                    purc_variant_object_set_by_static_ckey (ret_var, "basename", val);
                    purc_variant_unref (val);
                    break;
                }
                
                //printf ("ext_begin: %s, ext_length=%ld\n", ext_begin, ext_length);
                if (strcmp_len (flag, "extension", &flag_len) == 0) {
                    val = ext_begin ?
                            purc_variant_make_string_ex (ext_begin, ext_length, true)
                          : purc_variant_make_null();
                    purc_variant_object_set_by_static_ckey (ret_var, "extension", val);
                    purc_variant_unref (val);
                    break;
                }

                //printf ("base_begin: %s, fname_length=%ld\n", base_begin, fname_length);
                if (strcmp_len (flag, "filename", &flag_len) == 0) {
                    val = base_begin ?
                            purc_variant_make_string_ex (base_begin, fname_length, true)
                          : purc_variant_make_null();
                    purc_variant_object_set_by_static_ckey (ret_var, "filename", val);
                    purc_variant_unref (val);
                    break;
                }
                break;

            default:
                purc_variant_unref (ret_var);
                purc_set_error (PURC_ERROR_WRONG_STAGE);
                goto failed;
        }

        if (0 == flag_len)
            break;

        flag += flag_len;
    }

    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
readlink_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    char buffer[PATH_MAX] = {};
    const char *string_path = NULL;
    ssize_t nbytes;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the file name
    string_path = purc_variant_get_string_const (argv[0]);
    if (NULL == string_path) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    nbytes = readlink(string_path, buffer, sizeof(buffer));
    if (nbytes == -1) {
        set_purc_error_by_errno ();
        return purc_variant_make_boolean (false);
    }
    else {
        ret_var = purc_variant_make_string (buffer, true);
    }

    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
realpath_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    char resolved_path[PATH_MAX];
    const char *string_path = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the file name
    string_path = purc_variant_get_string_const (argv[0]);
    if (NULL == string_path) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (NULL == realpath(string_path, resolved_path)) {
        set_purc_error_by_errno ();
        goto failed;
    }
    else {
        ret_var = purc_variant_make_string (resolved_path, true);
    }

    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
rename_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *string_from = NULL;
    const char *string_to = NULL;

    if (nr_args < 2) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the file name
    string_from = purc_variant_get_string_const (argv[0]);
    string_to = purc_variant_get_string_const (argv[1]);
    if (NULL == string_from || NULL == string_to) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (0 == rename(string_from, string_to)) {
        return purc_variant_make_boolean (true);
    }

    set_purc_error_by_errno ();

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
rmdir_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *filename = NULL;
    DIR *dirp;
    struct dirent *dp;
    struct stat dir_stat;
    bool empty = true;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);
    if (NULL == filename) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (access(filename, F_OK | R_OK) != 0) {
        set_purc_error_by_errno ();
        goto failed;
    }

    if (stat(filename, &dir_stat) < 0) {
        set_purc_error_by_errno ();
        goto failed;
    }

    if (S_ISDIR(dir_stat.st_mode)) {
        dirp = opendir(filename);

        while ((dp = readdir(dirp)) != NULL) {
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
        return purc_variant_make_boolean (true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
stat_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    return get_stat_result (FN_OPTION_STAT, nr_args, argv, call_flags);
}

static purc_variant_t
symlink_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *string_target = NULL;
    const char *string_link = NULL;

    if (nr_args < 2) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the parameters
    string_target = purc_variant_get_string_const (argv[0]);
    string_link = purc_variant_get_string_const (argv[1]);
    if (NULL == string_target || NULL == string_link) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (symlink(string_target, string_link) == -1) {
        set_purc_error_by_errno ();
        goto failed;
    }

    return purc_variant_make_boolean (true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

#define TEMP_TEMPLATE      "purc-XXXXXX"
#define TEMP_TEMPLATE_LEN  (sizeof(TEMP_TEMPLATE) - 1)

static purc_variant_t
tempname_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    char filename[PATH_MAX + 1];
    const char *string_directory = NULL;
    const char *string_prefix = NULL;
    size_t dir_length = 0;
    size_t prefix_length = 0;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the parameters
    string_directory = purc_variant_get_string_const (argv[0]);
    if (NULL == string_directory) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }
    dir_length = strlen (string_directory);

    if (nr_args > 1) {
        string_prefix = purc_variant_get_string_const (argv[1]);
        if (NULL == string_prefix) {
            purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }
        prefix_length = strlen (string_prefix);
    }

    if ((dir_length + prefix_length + TEMP_TEMPLATE_LEN + 1) >= PATH_MAX) {
        purc_set_error (PURC_ERROR_TOO_LONG);
        goto failed;
    }

    strncpy (filename, string_directory, sizeof(filename)-1);
    if (filename[dir_length - 1] != '\\' &&
        filename[dir_length - 1] != '/') {
            filename[dir_length] = '/';
            filename[dir_length + 1] = '\0';
            dir_length += 1;
    }
    if ((dir_length + prefix_length + TEMP_TEMPLATE_LEN + 1) >= PATH_MAX) {
        purc_set_error (PURC_ERROR_TOO_LONG);
        goto failed;
    }

    if (string_prefix) {
        strncat (filename, string_prefix, sizeof(filename) - 1);
    }
    strncat (filename, TEMP_TEMPLATE, sizeof(filename) - 1);

    int tmp_fd;
    if ((tmp_fd = mkstemp (filename)) == -1) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        goto failed;
    }

    close(tmp_fd);
    return purc_variant_make_string (filename, true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
touch_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *filename = NULL;
    uint64_t mtime = UTIME_NOW;
    uint64_t atime = UTIME_NOW;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);
    if (NULL == filename) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }
    if (nr_args > 1) {
        if (PURC_VARIANT_TYPE_ULONGINT == argv[1]->type) {
            mtime = argv[1]->u64;
            atime = argv[1]->u64;
        }
        else {
            purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }
    }
    if (nr_args > 2) {
        if (PURC_VARIANT_TYPE_ULONGINT == argv[2]->type) {
            atime = argv[2]->u64;
        }
        else {
            purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }
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
        else {
            set_purc_error_by_errno ();
            goto failed;
        }
    }
    else {      // change time
        struct timespec newtime[2];
        newtime[0].tv_nsec = atime;
        newtime[1].tv_nsec = mtime;
        if (utimensat(AT_FDCWD, filename, newtime, 0) == 0) {
            ret_var = purc_variant_make_boolean (true);
        }
        else {
            purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
            goto failed;
        }
    }

    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
umask_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    mode_t mask;
    mode_t old_mask;
    char *endptr;
    char umask_octal[5];
    const char *string_mask = NULL;

    if (nr_args < 1) {
        // get the current umask
        old_mask = umask (0777);
        mask = umask (old_mask);
    }
    else {
        string_mask = purc_variant_get_string_const (argv[0]);
        if (NULL == string_mask) {
            purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        if ('0' == string_mask[0]) {
            mask = strtol (string_mask, &endptr, 8);  /* Octal number */
        }
        else {
            mask = strtol (string_mask, &endptr, 10); /* Decimal */
        }

        //This system call always succeeds and the previous value of the mask is returned.
        //mask = umask (mask);        
    }

    snprintf (umask_octal, sizeof(umask_octal), "0%o", mask);
    return purc_variant_make_string (umask_octal, true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
unlink_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *filename = NULL;
    struct stat filestat;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);
    if (NULL == filename) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (access(filename, F_OK | R_OK) != 0) {
        set_purc_error_by_errno ();
        goto failed;
    }

    if (stat(filename, &filestat) < 0) {
        set_purc_error_by_errno ();
        goto failed;
    }

    if (S_ISREG(filestat.st_mode)) {
        if (unlink (filename) == 0) {
            return purc_variant_make_boolean (true);
        }
        else {
            set_purc_error_by_errno ();
            return purc_variant_make_boolean (false);
        }
    }

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
rm_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *filename = NULL;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);
    if (NULL == filename) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (remove_dir ((char *)filename) == 0)
        return purc_variant_make_boolean (true);
    
    set_purc_error_by_errno ();
    return purc_variant_make_boolean (false);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
file_contents_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *string_filename = NULL;
    const char *string_flags = "";
    const char *flag;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    bool        flag_binary = false;
    bool        flag_strict = false;
    int64_t     offset = 0;
    size_t      length = SIZE_MAX;
    struct stat filestat;
    size_t      filesize;
    size_t      readsize;
    FILE       *fp = NULL;
    uint8_t    *bsequence = NULL;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (NULL == string_filename) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (nr_args > 1) {
        string_flags = purc_variant_get_string_const (argv[1]);
        if (NULL == string_flags) {
            purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }
    }

    if (nr_args > 2) {
        // Get the offset
        if (! purc_variant_cast_to_longint (argv[2], &offset, false)) {
            purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }
    }

    if (nr_args > 3) {
        // Get the length
        uint64_t len;
        if (! purc_variant_cast_to_ulongint (argv[3], &len, false)) {
            purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        length = len;
    }

    // Parse flags
    flag = string_flags;
    while (*flag)
    {
        size_t flag_len = 0;

        while (purc_isspace(*flag))
            flag ++;

        if (strcmp_len (flag, "binary", &flag_len) == 0) {
            flag_binary = true;
        }
        else if (strcmp_len (flag, "string", &flag_len) == 0) {
            flag_binary = false;
        }
        else if (strcmp_len (flag, "strict", &flag_len) == 0) {
            flag_strict = true;
        }
        else if (strcmp_len (flag, "silent", &flag_len) == 0) {
            flag_strict = false;
        }
        else {
            purc_variant_unref (ret_var);
            purc_set_error (PURC_ERROR_WRONG_STAGE);
            goto failed;
        }

        if (0 == flag_len)
            break;

        flag += flag_len;
    }

    // Get whole file size
    if (stat(string_filename, &filestat) < 0) {
        purc_set_error (PURC_ERROR_NOT_EXISTS);
        goto failed;
    }
    filesize = filestat.st_size;

    if (offset < 0) {
        offset = filesize + offset;// offset < 0 !!!
    }
    if (offset < 0 || (size_t)offset >= filesize) {
        purc_set_error (PURC_ERROR_WRONG_STAGE);
        goto failed;
    }

    if ((filesize - offset) < length)
        length = filesize - offset;

    if (length <= 0) {
        purc_set_error (PURC_ERROR_WRONG_STAGE);
        goto failed;
    }

    bsequence = malloc (length + 1);
    bsequence[length] = 0x0;

    if (flag_binary)
        fp = fopen (string_filename, "rb");
    else
        fp = fopen (string_filename, "r");

    if (NULL == fp) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        goto failed;
    }

    if (offset > 0)
        fseek (fp, offset, SEEK_SET);

    readsize = fread (bsequence, 1, length, fp);
    if (readsize != length && flag_strict) {
        // throw `BadEncoding` exception
        purc_set_error (PURC_ERROR_BAD_ENCODING);
        goto failed;
    }

    if (flag_binary)
        ret_var = purc_variant_make_byte_sequence(bsequence, readsize);
    else
        ret_var = purc_variant_make_string_ex((const char *)bsequence, readsize, true);

    free (bsequence);
    fclose (fp);
    return ret_var;

failed:
    if (bsequence)
        free (bsequence);

    if (fp)
        fclose (fp);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
file_contents_setter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *string_filename = NULL;
    const char *string_flags = "";
    const char *flag;
    const char *string_content;
    FILE       *fp = NULL;
    const uint8_t *bsequence = NULL;
    size_t      sz_bseq;
    size_t      writesize;
    bool        flag_lock = false;
    bool        flag_append = false;
    bool        flag_binary = false;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (NULL == string_filename) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (nr_args > 1) {
        string_content = purc_variant_get_string_const (argv[1]);

        if (NULL == string_content) {
            if (! purc_variant_cast_to_byte_sequence(argv[1],
                    (const void **)&bsequence, &sz_bseq))
            {
                purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
                goto failed;
            }

            flag_binary = true;
        }
    }

    if (nr_args > 2) {
        string_flags = purc_variant_get_string_const (argv[2]);
        if (NULL == string_flags) {
            purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }
    }

    // Parse flags
    flag = string_flags;
    while (*flag)
    {
        size_t flag_len = 0;

        while (purc_isspace(*flag))
            flag ++;

        if (strcmp_len (flag, "lock", &flag_len) == 0) {
            flag_lock = true;
        }
        else if (strcmp_len (flag, "append", &flag_len) == 0) {
            flag_append = true;
        }
        if (strcmp_len (flag, "binary", &flag_len) == 0) {
            flag_binary = true;
        }
        else if (strcmp_len (flag, "string", &flag_len) == 0) {
            flag_binary = false;
        }
        else {
            purc_set_error (PURC_ERROR_WRONG_STAGE);
            goto failed;
        }

        if (0 == flag_len)
            break;

        flag += flag_len;
    }

    if (flag_binary) {
        if (flag_append)
            fp = fopen (string_filename, "ab");
        else
            fp = fopen (string_filename, "wb");
    }
    else {
        if (flag_append)
            fp = fopen (string_filename, "a");
        else
            fp = fopen (string_filename, "w");
    }

    if (NULL == fp) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        goto failed;
    }

    if (-1 == flock (fileno(fp), LOCK_SH | LOCK_NB)) {
        // File locked.
        purc_set_error (PURC_ERROR_ACCESS_DENIED);
        goto failed;
    }

    flock(fileno(fp), LOCK_UN);

    if (flag_lock) {
        if (flock (fileno(fp), LOCK_EX) != 0) {
            purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
            goto failed;
        }
    }

    if (flag_binary)
        writesize = fwrite (string_content, 1, strlen(string_content), fp);
    else
        writesize = fwrite (bsequence, 1, sz_bseq, fp);

    if (flag_lock) {
        flock (fileno(fp), LOCK_UN);
    }
    fclose (fp);
    return purc_variant_make_ulongint (writesize);

failed:
    if (fp)
        fclose (fp);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
on_dir_read (void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    DIR *dirp = (DIR *)native_entity;
    struct dirent *dp;

    if (NULL == dirp) {
        goto failed;
    }

    while ((dp = readdir(dirp)) != NULL) {
        if ((strcmp(dp->d_name, ".") == 0)
                || (strcmp(dp->d_name, "..") == 0))
            continue;

        return purc_variant_make_string (dp->d_name, true);
    }

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
on_dir_rewind (void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    if (native_entity) {
        rewinddir((DIR *)native_entity);
        return purc_variant_make_boolean (true);
    }

    return purc_variant_make_boolean (false);
}

static purc_nvariant_method
property_getter(void* native_entity, const char* key_name)
{
    UNUSED_PARAM(native_entity);

    if (key_name) {
        switch (key_name[0]) {
        case 'r':
            if (strcmp(key_name, "read") == 0) {
                return on_dir_read;
            }
            if (strcmp(key_name, "rewind") == 0) {
                return on_dir_rewind;
            }
            break;

        default:
            break;
        }
    }

    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

static void
on_release(void *native_entity)
{
    if (native_entity) {
        closedir((DIR *)native_entity);
    }
}

static purc_variant_t
opendir_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *string_pathname = NULL;
    DIR *dirp;
    struct stat dir_stat;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the file name
    string_pathname = purc_variant_get_string_const (argv[0]);
    if (NULL == string_pathname) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (access (string_pathname, F_OK | R_OK) != 0) {
        purc_set_error (PURC_ERROR_NOT_EXISTS);
        goto failed;
    }

    if (stat (string_pathname, &dir_stat) < 0) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        goto failed;
    }

    if (S_ISDIR(dir_stat.st_mode)) {
        dirp = opendir (string_pathname);
        if (NULL == dirp) {
            purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
            goto failed;
        }
    }
    else {
        goto failed;
    }

    static const struct purc_native_ops ops = {
        .property_getter = property_getter,
        .on_observe = NULL,
        .on_forget = NULL,
        .on_release = on_release,
    };

    ret_var = purc_variant_make_native((void *)dirp, &ops);
    if (ret_var == PURC_VARIANT_INVALID) {
        goto failed;
    }

    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
closedir_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    DIR *dirp;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    dirp = (DIR *)purc_variant_native_get_entity(argv[0]);
    if (NULL == dirp) {
        purc_set_error (PURC_ERROR_NO_DATA);
        goto failed;
    }

    if (0 == closedir(dirp))
        ret_var = purc_variant_make_boolean (true);
    else {
        purc_set_error (PURC_ERROR_SYS_FAULT);
        goto failed;
    }

    argv[0]->ptr_ptr[0] = NULL;
    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean (false);

    return PURC_VARIANT_INVALID;
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
        {"file_contents", file_contents_getter, file_contents_setter},
        {"opendir",       opendir_getter, NULL},
        {"closedir",      closedir_getter, NULL}
    };

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
