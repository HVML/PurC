/**
 * @file fs.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for file system.
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

#ifndef PCHTML_FS_H
#define PCHTML_FS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/base.h"

typedef pchtml_action_t (*pchtml_fs_dir_file_f)(const unsigned char *fullpath,
                                                size_t fullpath_len,
                                                const unsigned char *filename,
                                                size_t filename_len, void *ctx);

typedef int pchtml_fs_dir_opt_t;

enum pchtml_fs_dir_opt {
    PCHTML_FS_DIR_OPT_UNDEF          = 0x00,
    PCHTML_FS_DIR_OPT_WITHOUT_DIR    = 0x01,
    PCHTML_FS_DIR_OPT_WITHOUT_FILE   = 0x02,
    PCHTML_FS_DIR_OPT_WITHOUT_HIDDEN = 0x04,
};

typedef enum {
    PCHTML_FS_FILE_TYPE_UNDEF            = 0x00,
    PCHTML_FS_FILE_TYPE_FILE             = 0x01,
    PCHTML_FS_FILE_TYPE_DIRECTORY        = 0x02,
    PCHTML_FS_FILE_TYPE_BLOCK_DEVICE     = 0x03,
    PCHTML_FS_FILE_TYPE_CHARACTER_DEVICE = 0x04,
    PCHTML_FS_FILE_TYPE_PIPE             = 0x05,
    PCHTML_FS_FILE_TYPE_SYMLINK          = 0x06,
    PCHTML_FS_FILE_TYPE_SOCKET           = 0x07
}
pchtml_fs_file_type_t;


unsigned int
pchtml_fs_dir_read(const unsigned char *dirpath, pchtml_fs_dir_opt_t opt,
                   pchtml_fs_dir_file_f callback, void *ctx) WTF_INTERNAL;

pchtml_fs_file_type_t
pchtml_fs_file_type(const unsigned char *full_path) WTF_INTERNAL;

unsigned char *
pchtml_fs_file_easy_read(const unsigned char *full_path, size_t *len) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_FS_H */

