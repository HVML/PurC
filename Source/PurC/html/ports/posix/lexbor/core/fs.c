/**
 * @file fs.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of file system.
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

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#include <string.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "html/core/fs.h"


unsigned int
pchtml_fs_dir_read(const unsigned char *dirpath, pchtml_fs_dir_opt_t opt,
                   pchtml_fs_dir_file_f callback, void *ctx)
{
    DIR *dir;
    size_t path_len, free_len, d_namlen;
    struct dirent *entry;
    pchtml_action_t action;
    pchtml_fs_file_type_t f_type;

    char *file_begin;
    char full_path[4096];

    path_len = strlen((const char *) dirpath);
    if (path_len == 0 || path_len >= (sizeof(full_path) - 1)) {
        return PCHTML_STATUS_ERROR;
    }

    dir = opendir((const char *) dirpath);
    if (dir == NULL) {
        return PCHTML_STATUS_ERROR;
    }

    memcpy(full_path, dirpath, path_len);

    /* Check for a separating character at the end dirpath */
    if (full_path[(path_len - 1)] != '/') {
        path_len++;

        if (path_len >= (sizeof(full_path) - 1)) {
            goto error;
        }

        full_path[(path_len - 1)] = '/';
    }

    file_begin = &full_path[path_len];
    free_len = (sizeof(full_path) - 1) - path_len;

    if (opt == PCHTML_FS_DIR_OPT_UNDEF)
    {
        while ((entry = readdir(dir)) != NULL) {
            d_namlen = strlen(entry->d_name);

            if (d_namlen >= free_len) {
                goto error;
            }

            /* +1 copy terminating null byte '\0' */
            memcpy(file_begin, entry->d_name, (d_namlen + 1));

            action = callback((const unsigned char *) full_path,
                              (path_len + d_namlen),
                              (const unsigned char *) entry->d_name,
                              d_namlen, ctx);
            if (action == PCHTML_ACTION_STOP) {
                break;
            }
        }

        goto done;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (opt & PCHTML_FS_DIR_OPT_WITHOUT_HIDDEN
            && *entry->d_name == '.')
        {
            continue;
        }

        d_namlen = strlen(entry->d_name);

        if (d_namlen >= free_len) {
            goto error;
        }

        f_type = pchtml_fs_file_type((const unsigned char *) entry->d_name);

        if (opt & PCHTML_FS_DIR_OPT_WITHOUT_DIR
            && f_type == PCHTML_FS_FILE_TYPE_DIRECTORY)
        {
            continue;
        }

        if (opt & PCHTML_FS_DIR_OPT_WITHOUT_FILE
            && f_type == PCHTML_FS_FILE_TYPE_FILE)
        {
            continue;
        }

        /* +1 copy terminating null byte '\0' */
        memcpy(file_begin, entry->d_name, (d_namlen + 1));

        action = callback((const unsigned char *) full_path,
                          (path_len + d_namlen),
                          (const unsigned char *) entry->d_name,
                          d_namlen, ctx);
        if (action == PCHTML_ACTION_STOP) {
            break;
        }
    }

done:

    closedir(dir);

    return PCHTML_STATUS_OK;

error:

    closedir(dir);

    return PCHTML_STATUS_ERROR;
}

pchtml_fs_file_type_t
pchtml_fs_file_type(const unsigned char *full_path)
{
    struct stat sb;

    if (stat((const char *) full_path, &sb) == -1) {
        return PCHTML_FS_FILE_TYPE_UNDEF;
    }

    switch (sb.st_mode & S_IFMT) {
        case S_IFBLK:
            return PCHTML_FS_FILE_TYPE_BLOCK_DEVICE;

        case S_IFCHR:
            return PCHTML_FS_FILE_TYPE_CHARACTER_DEVICE;

        case S_IFDIR:
            return PCHTML_FS_FILE_TYPE_DIRECTORY;

        case S_IFIFO:
            return PCHTML_FS_FILE_TYPE_PIPE;

        case S_IFLNK:
            return PCHTML_FS_FILE_TYPE_SYMLINK;

        case S_IFREG:
            return PCHTML_FS_FILE_TYPE_FILE;

        case S_IFSOCK:
            return PCHTML_FS_FILE_TYPE_SOCKET;

        default:
            return PCHTML_FS_FILE_TYPE_UNDEF;
    }

    return PCHTML_FS_FILE_TYPE_UNDEF;
}

unsigned char *
pchtml_fs_file_easy_read(const unsigned char *full_path, size_t *len)
{
    FILE *fh;
    long size;
    size_t nread;
    unsigned char *data;

    fh = fopen((const char *) full_path, "rb");
    if (fh == NULL) {
        goto error;
    }

    if (fseek(fh, 0L, SEEK_END) != 0) {
        goto error_close;
    }

    size = ftell(fh);
    if (size < 0) {
        goto error_close;
    }

    if (fseek(fh, 0L, SEEK_SET) != 0) {
        goto error_close;
    }

    data = pchtml_malloc(size + 1);
    if (data == NULL) {
        goto error_close;
    }

    nread = fread(data, 1, size, fh);
    if (nread != (size_t) size) {
        pchtml_free(data);

        goto error_close;
    }

    fclose(fh);

    data[size] = '\0';

    if (len != NULL) {
        *len = nread;
    }

    return data;

error_close:

    fclose(fh);

error:

    if (len != NULL) {
        *len = 0;
    }

    return NULL;
}
