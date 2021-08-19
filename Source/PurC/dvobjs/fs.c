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

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/edom.h"
#include "private/html.h"
#include "purc-variant.h"

#include <unistd.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

static const char* get_work_dirctory (void)
{
    return "/home/gengyue";
}

static bool remove_dir (char * dir)
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
            if ((strcmp(dp->d_name, ".") == 0) || (strcmp(dp->d_name, "..") == 0)) 
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
list_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return NULL;
}

static purc_variant_t
list_prt_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return NULL;
}

static purc_variant_t
mkdir_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    char filename[PATH_MAX] = {0,};
    const char* string_filename = NULL;
    purc_variant_t ret_var = NULL;

    if ((argv == NULL) || (nr_args != 1)) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (*string_filename == '/') {
        strcpy (filename, string_filename);
    }
    else {
        strcpy (filename, get_work_dirctory ());
        strcat (filename, "/");
        strcat (filename, string_filename);
    }

    if (access(filename, F_OK | R_OK) == 0) 
        return purc_variant_make_boolean (false);

    if (mkdir (filename, 777) == 0)
        ret_var = purc_variant_make_boolean (false);
    else
        ret_var = purc_variant_make_boolean (true);

    return ret_var;
}


static purc_variant_t
rmdir_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    char filename[PATH_MAX] = {0,};
    const char* string_filename = NULL;
    DIR *dirp;
    struct dirent *dp;
    struct stat dir_stat;
    purc_variant_t ret_var = NULL;
    bool empty = false;

    if ((argv == NULL) || (nr_args != 1)) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (*string_filename == '/') {
        strcpy (filename, string_filename);
    }
    else {
        strcpy (filename, get_work_dirctory ());
        strcat (filename, "/");
        strcat (filename, string_filename);
    }

    if (access(filename, F_OK | R_OK) != 0) 
        return purc_variant_make_boolean (false);

    if (stat(filename, &dir_stat) < 0) 
        return purc_variant_make_boolean (false);

    if (S_ISDIR(dir_stat.st_mode)) {
        dirp = opendir(filename);

        while ((dp=readdir(dirp)) != NULL) {
            if ((strcmp(dp->d_name, ".") == 0) || (strcmp(dp->d_name, "..") == 0)) 
                continue;
            else {
                break;
            }
        }
        closedir(dirp);

        if (empty) {
            if (rmdir(filename) == 0)
                empty = true;
        }
    } 

    if (empty)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
touch_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    char filename[PATH_MAX] = {0,};
    const char* string_filename = NULL;
    purc_variant_t ret_var = NULL;

    if ((argv == NULL) || (nr_args != 1)) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (*string_filename == '/') {
        strcpy (filename, string_filename);
    }
    else {
        strcpy (filename, get_work_dirctory ());
        strcat (filename, "/");
        strcat (filename, string_filename);
    }

    return ret_var;
}


static purc_variant_t
unlink_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    char filename[PATH_MAX] = {0,};
    const char* string_filename = NULL;
    struct stat filestat;
    purc_variant_t ret_var = NULL;

    if ((argv == NULL) || (nr_args != 1)) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (*string_filename == '/') {
        strcpy (filename, string_filename);
    }
    else {
        strcpy (filename, get_work_dirctory ());
        strcat (filename, "/");
        strcat (filename, string_filename);
    }

    if (access(filename, F_OK | R_OK) == 0) 
        return purc_variant_make_boolean (false);

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
rm_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    char filename[PATH_MAX] = {0,};
    const char* string_filename = NULL;
    purc_variant_t ret_var = NULL;

    if ((argv == NULL) || (nr_args != 1)) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (*string_filename == '/') {
        strcpy (filename, string_filename);
    }
    else {
        strcpy (filename, get_work_dirctory ());
        strcat (filename, "/");
        strcat (filename, string_filename);
    }

    if (remove_dir (filename))
        ret_var = purc_variant_make_boolean (false);
    else
        ret_var = purc_variant_make_boolean (true);

    return ret_var;

}
// only for test now.
purc_variant_t pcdvojbs_get_fs (void)
{
    purc_variant_t fs = purc_variant_make_object_c (7,
            "list",      purc_variant_make_dynamic (list_getter, NULL),
            "list_prt",  purc_variant_make_dynamic (list_prt_getter, NULL),
            "mkdir",     purc_variant_make_dynamic (mkdir_getter, NULL),
            "rmdir",     purc_variant_make_dynamic (rmdir_getter, NULL),
            "touch",     purc_variant_make_dynamic (touch_getter, NULL),
            "unlink",    purc_variant_make_dynamic (unlink_getter, NULL),
            "rm",        purc_variant_make_dynamic (rm_getter, NULL)
       );
    return fs;
}

