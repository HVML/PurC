/*
   Virtual File System path handlers

   Copyright (C) 2011-2021
   Free Software Foundation, Inc.

   Written by:
   Slava Zanko <slavazanko@gmail.com>, 2011, 2013
   Andrew Borodin <aborodin@vmail.ru>, 2013

   This file is part of the Midnight Commander.

   The Midnight Commander is free software: you can redistribute it
   and/or modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   The Midnight Commander is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 * \brief Source: Virtual File System: path handlers
 * \author Slava Zanko
 * \date 2011
 */


#include <config.h>

#include "lib/global.h"
#include "lib/strutil.h"
#include "lib/util.h"           /* mc_build_filename() */
#include "lib/serialize.h"

#include "vfs.h"
#include "utilvfs.h"
#include "xdirentry.h"
#include "path.h"

extern GPtrArray *vfs__classes_list;

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static gboolean
path_magic (const char *path)
{
    struct stat buf;

    return (stat (path, &buf) != 0);
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Splits path extracting vfs part.
 *
 * Splits path
 * \verbatim /p1#op/inpath \endverbatim
 * into
 * \verbatim inpath,op; \endverbatim
 * returns which vfs it is.
 * What is left in path is p1. You still want to g_free(path), you DON'T
 * want to free neither *inpath nor *op
 */

static struct vfs_class *
_vfs_split_with_semi_skip_count (char *path, const char **inpath, const char **op,
                                 size_t skip_count)
{
    char *semi;
    char *slash;
    struct vfs_class *ret;

    if (path == NULL)
        vfs_die ("Cannot split NULL");

    semi = strrstr_skip_count (path, "#", skip_count);

    if ((semi == NULL) || (!path_magic (path)))
        return NULL;

    slash = strchr (semi, PATH_SEP);
    *semi = '\0';

    if (op != NULL)
        *op = NULL;

    if (inpath != NULL)
        *inpath = NULL;

    if (slash != NULL)
        *slash = '\0';

    ret = vfs_prefix_to_class (semi + 1);
    if (ret != NULL)
    {
        if (op != NULL)
            *op = semi + 1;
        if (inpath != NULL)
            *inpath = slash != NULL ? slash + 1 : NULL;
        return ret;
    }

    if (slash != NULL)
        *slash = PATH_SEP;

    *semi = '#';
    ret = _vfs_split_with_semi_skip_count (path, inpath, op, skip_count + 1);
    return ret;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * remove //, /./ and /../
 *
 * @return newly allocated string
 */

static char *
vfs_canon (const char *path)
{
    char *result;

    if (path == NULL)
        vfs_die ("Cannot canonicalize NULL");

    if (!IS_PATH_SEP (*path))
    {
        /* Relative to current directory */

        char *local;

        if (g_str_has_prefix (path, VFS_ENCODING_PREFIX))
        {
            /*
               encoding prefix placed at start of string without the leading slash
               should be autofixed by adding the leading slash
             */
            local = mc_build_filename (PATH_SEP_STR, path, (char *) NULL);
        }
        else
        {
            const char *curr_dir;

            curr_dir = vfs_get_current_dir ();
            local = mc_build_filename (curr_dir, path, (char *) NULL);
        }
        result = vfs_canon (local);
        g_free (local);
    }
    else
    {
        /* Absolute path */

        result = g_strdup (path);
        canonicalize_pathname (result);
    }

    return result;
}

/* --------------------------------------------------------------------------------------------- */

#ifdef HAVE_CHARSET
/** get encoding after last #enc: or NULL, if part does not contain #enc:
 *
 * @param path null-terminated string
 * @param len the maximum length of path, where #enc: should be searched
 *
 * @return newly allocated string.
 */

static char *
vfs_get_encoding (const char *path, ssize_t len)
{
    char *semi;

    /* try found #enc: */
    semi = g_strrstr_len (path, len, VFS_ENCODING_PREFIX);
    if (semi == NULL)
        return NULL;

    if (semi == path || IS_PATH_SEP (semi[-1]))
    {
        char *slash;

        semi += strlen (VFS_ENCODING_PREFIX);   /* skip "#enc:" */
        slash = strchr (semi, PATH_SEP);
        if (slash != NULL)
            return g_strndup (semi, slash - semi);
        return g_strdup (semi);
    }

    return vfs_get_encoding (path, semi - path);
}
#endif

/* --------------------------------------------------------------------------------------------- */
/**  Extract the hostname and username from the path
 *
 * Format of the path is [user@]hostname:port/remote-dir, e.g.:
 *
 * ftp://sunsite.unc.edu/pub/linux
 * ftp://miguel@sphinx.nuclecu.unam.mx/c/nc
 * ftp://tsx-11.mit.edu:8192/
 * ftp://joe@foo.edu:11321/private
 * ftp://joe:password@foo.se
 *
 * @param path_element is an input string to be parsed
 * @param path is an input string to be parsed
 *
 * @return g_malloc()ed url info.
 *         If the user is empty, e.g. ftp://@roxanne/private, and URL_USE_ANONYMOUS
 *         is not set, then the current login name is supplied.
 *         Return value is a g_malloc()ed structure with the pathname relative to the
 *         host.
 */

static void
vfs_path_url_split (vfs_path_element_t * path_element, const char *path)
{
    char *pcopy;
    char *colon, *at, *rest;

    path_element->port = 0;

    pcopy = g_strdup (path);

    /* search for any possible user */
    at = strrchr (pcopy, '@');

    /* We have a username */
    if (at == NULL)
        rest = pcopy;
    else
    {
        const char *pend;
        char *inner_colon;

        pend = strchr (at, '\0');
        *at = '\0';

        inner_colon = strchr (pcopy, ':');
        if (inner_colon != NULL)
        {
            *inner_colon = '\0';
            inner_colon++;
            path_element->password = g_strdup (inner_colon);
        }

        if (*pcopy != '\0')
            path_element->user = g_strdup (pcopy);

        if (pend == at + 1)
            rest = at;
        else
            rest = at + 1;
    }

    /* Check if the host comes with a port spec, if so, chop it */
    if (*rest != '[')
        colon = strchr (rest, ':');
    else
    {
        colon = strchr (++rest, ']');
        if (colon != NULL)
        {
            *colon = '\0';
            colon++;
            *colon = '\0';
            path_element->ipv6 = TRUE;
        }
    }

    if (colon != NULL)
    {
        *colon = '\0';
        /* cppcheck-suppress invalidscanf */
        if (sscanf (colon + 1, "%d", &path_element->port) == 1)
        {
            if (path_element->port <= 0 || path_element->port >= 65536)
                path_element->port = 0;
        }
        else
            while (*(++colon) != '\0')
            {
                switch (*colon)
                {
                case 'C':
                    path_element->port = 1;
                    break;
                case 'r':
                    path_element->port = 2;
                    break;
                default:
                    break;
                }
            }
    }
    path_element->host = g_strdup (rest);
    g_free (pcopy);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * get VFS class for the given name
 *
 * @param class_name name of class
 *
 * @return pointer to class structure or NULL if class not found
 */

static struct vfs_class *
vfs_get_class_by_name (const char *class_name)
{
    guint i;

    if (class_name == NULL)
        return NULL;

    for (i = 0; i < vfs__classes_list->len; i++)
    {
        struct vfs_class *vfs = VFS_CLASS (g_ptr_array_index (vfs__classes_list, i));
        if ((vfs->name != NULL) && (strcmp (vfs->name, class_name) == 0))
            return vfs;
    }

    return NULL;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Check if path string contain URL-like elements
 *
 * @param path_str path
 *
 * @return TRUE if path is deprecated or FALSE otherwise
 */

static gboolean
vfs_path_is_str_path_deprecated (const char *path_str)
{
    return strstr (path_str, VFS_PATH_URL_DELIMITER) == NULL;
}

/* --------------------------------------------------------------------------------------------- */
/** Split path string to path elements by deprecated algorithm.
 *
 * @param path_str VFS-path
 *
 * @return pointer to newly created vfs_path_t object with filled path elements array.
*/

static vfs_path_t *
vfs_path_from_str_deprecated_parser (char *path)
{
    vfs_path_t *vpath;
    vfs_path_element_t *element;
    struct vfs_class *class;
    const char *local, *op;

    vpath = vfs_path_new ();

    while ((class = _vfs_split_with_semi_skip_count (path, &local, &op, 0)) != NULL)
    {
        char *url_params;
        element = g_new0 (vfs_path_element_t, 1);
        element->class = class;
        if (local == NULL)
            local = "";
        element->path = vfs_translate_path_n (local);

#ifdef HAVE_CHARSET
        element->encoding = vfs_get_encoding (local, -1);
        element->dir.converter =
            (element->encoding != NULL) ? str_crt_conv_from (element->encoding) : INVALID_CONV;
#endif

        url_params = strchr (op, ':');  /* skip VFS prefix */
        if (url_params != NULL)
        {
            *url_params = '\0';
            url_params++;
            vfs_path_url_split (element, url_params);
        }

        if (*op != '\0')
            element->vfs_prefix = g_strdup (op);

        g_array_prepend_val (vpath->path, element);
    }
    if (path[0] != '\0')
    {
        element = g_new0 (vfs_path_element_t, 1);
        element->class = g_ptr_array_index (vfs__classes_list, 0);
        element->path = vfs_translate_path_n (path);

#ifdef HAVE_CHARSET
        element->encoding = vfs_get_encoding (path, -1);
        element->dir.converter =
            (element->encoding != NULL) ? str_crt_conv_from (element->encoding) : INVALID_CONV;
#endif
        g_array_prepend_val (vpath->path, element);
    }

    return vpath;
}

/* --------------------------------------------------------------------------------------------- */
/** Split path string to path elements by URL algorithm.
 *
 * @param path_str VFS-path
 * @param flags    flags for converter
 *
 * @return pointer to newly created vfs_path_t object with filled path elements array.
*/

static vfs_path_t *
vfs_path_from_str_uri_parser (char *path)
{
    vfs_path_t *vpath;
    vfs_path_element_t *element;
    char *url_delimiter;

    vpath = vfs_path_new ();
    vpath->relative = path != NULL && !IS_PATH_SEP (*path);

    while ((url_delimiter = g_strrstr (path, VFS_PATH_URL_DELIMITER)) != NULL)
    {
        char *vfs_prefix_start;
        char *real_vfs_prefix_start = url_delimiter;

        while (real_vfs_prefix_start > path && !IS_PATH_SEP (*real_vfs_prefix_start))
            real_vfs_prefix_start--;
        vfs_prefix_start = real_vfs_prefix_start;

        if (IS_PATH_SEP (*vfs_prefix_start))
            vfs_prefix_start += 1;

        *url_delimiter = '\0';

        element = g_new0 (vfs_path_element_t, 1);
        element->class = vfs_prefix_to_class (vfs_prefix_start);
        element->vfs_prefix = g_strdup (vfs_prefix_start);

        url_delimiter += strlen (VFS_PATH_URL_DELIMITER);

        if (element->class != NULL && (element->class->flags & VFSF_REMOTE) != 0)
        {
            char *slash_pointer;

            slash_pointer = strchr (url_delimiter, PATH_SEP);
            if (slash_pointer == NULL)
            {
                element->path = g_strdup ("");
            }
            else
            {
                element->path = vfs_translate_path_n (slash_pointer + 1);
#ifdef HAVE_CHARSET
                element->encoding = vfs_get_encoding (slash_pointer, -1);
#endif
                *slash_pointer = '\0';
            }
            vfs_path_url_split (element, url_delimiter);
        }
        else
        {
            element->path = vfs_translate_path_n (url_delimiter);
#ifdef HAVE_CHARSET
            element->encoding = vfs_get_encoding (url_delimiter, -1);
#endif
        }
#ifdef HAVE_CHARSET
        element->dir.converter =
            (element->encoding != NULL) ? str_crt_conv_from (element->encoding) : INVALID_CONV;
#endif
        g_array_prepend_val (vpath->path, element);

        if ((real_vfs_prefix_start > path && IS_PATH_SEP (*real_vfs_prefix_start)) ||
            (real_vfs_prefix_start == path && !IS_PATH_SEP (*real_vfs_prefix_start)))
            *real_vfs_prefix_start = '\0';
        else
            *(real_vfs_prefix_start + 1) = '\0';
    }

    if (path[0] != '\0')
    {
        element = g_new0 (vfs_path_element_t, 1);
        element->class = g_ptr_array_index (vfs__classes_list, 0);
        element->path = vfs_translate_path_n (path);
#ifdef HAVE_CHARSET
        element->encoding = vfs_get_encoding (path, -1);
        element->dir.converter =
            (element->encoding != NULL) ? str_crt_conv_from (element->encoding) : INVALID_CONV;
#endif
        g_array_prepend_val (vpath->path, element);
    }

    return vpath;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Add element's class info to result string (such as VFS name, host, encoding etc)
 * This function used as helper only in vfs_path_tokens_get() function
 *
 * @param element current path element
 * @param ret_tokens total tikens for return
 * @param element_tokens accumulated element-only tokens
 */

static void
vfs_path_tokens_add_class_info (const vfs_path_element_t * element, GString * ret_tokens,
                                GString * element_tokens)
{
    if (((element->class->flags & VFSF_LOCAL) == 0 || ret_tokens->len > 0)
        && element_tokens->len > 0)
    {
        char *url_str;

        if (ret_tokens->len > 0 && !IS_PATH_SEP (ret_tokens->str[ret_tokens->len - 1]))
            g_string_append_c (ret_tokens, PATH_SEP);

        g_string_append (ret_tokens, element->vfs_prefix);
        g_string_append (ret_tokens, VFS_PATH_URL_DELIMITER);

        url_str = vfs_path_build_url_params_str (element, TRUE);
        if (*url_str != '\0')
        {
            g_string_append (ret_tokens, url_str);
            g_string_append_c (ret_tokens, PATH_SEP);
        }

        g_free (url_str);
    }

#ifdef HAVE_CHARSET
    if (element->encoding != NULL)
    {
        if (ret_tokens->len > 0 && !IS_PATH_SEP (ret_tokens->str[ret_tokens->len - 1]))
            g_string_append (ret_tokens, PATH_SEP_STR);
        g_string_append (ret_tokens, VFS_ENCODING_PREFIX);
        g_string_append (ret_tokens, element->encoding);
        g_string_append (ret_tokens, PATH_SEP_STR);
    }
#endif

    g_string_append (ret_tokens, element_tokens->str);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Strip path to home dir.
 * @param dir pointer to string contains full path
 */

static char *
vfs_path_strip_home (const char *dir)
{
    const char *home_dir = mc_config_get_home_dir ();

    if (home_dir != NULL)
    {
        size_t len;

        len = strlen (home_dir);

        if (strncmp (dir, home_dir, len) == 0 && (IS_PATH_SEP (dir[len]) || dir[len] == '\0'))
            return g_strdup_printf ("~%s", dir + len);
    }

    return g_strdup (dir);
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

#define vfs_append_from_path(appendfrom, is_relative) \
{ \
    if ((flags & VPF_STRIP_HOME) && element_index == 0 && \
        (element->class->flags & VFSF_LOCAL) != 0) \
    { \
        char *stripped_home_str; \
        stripped_home_str = vfs_path_strip_home (appendfrom); \
        g_string_append (buffer, stripped_home_str); \
        g_free (stripped_home_str); \
    } \
    else \
    { \
        if (!is_relative && !IS_PATH_SEP (*appendfrom) && *appendfrom != '\0' \
            && (buffer->len == 0 || !IS_PATH_SEP (buffer->str[buffer->len - 1]))) \
            g_string_append_c (buffer, PATH_SEP); \
        g_string_append (buffer, appendfrom); \
    } \
}

/**
 * Convert first elements_count elements from vfs_path_t to string representation with flags.
 *
 * @param vpath pointer to vfs_path_t object
 * @param elements_count count of first elements for convert
 * @param flags for converter
 *
 * @return pointer to newly created string.
 */

char *
vfs_path_to_str_flags (const vfs_path_t * vpath, int elements_count, vfs_path_flag_t flags)
{
    int element_index;
    GString *buffer;
    GString *recode_buffer;

    if (vpath == NULL)
        return NULL;

    if (elements_count == 0 || elements_count > vfs_path_elements_count (vpath))
        elements_count = vfs_path_elements_count (vpath);

    if (elements_count < 0)
        elements_count = vfs_path_elements_count (vpath) + elements_count;

    buffer = g_string_new ("");
    recode_buffer = g_string_new ("");

    for (element_index = 0; element_index < elements_count; element_index++)
    {
        const vfs_path_element_t *element;
        gboolean is_relative = vpath->relative && (element_index == 0);

        element = vfs_path_get_by_index (vpath, element_index);
        if (element->vfs_prefix != NULL)
        {
            char *url_str;
            if (!is_relative && (buffer->len == 0 || !IS_PATH_SEP (buffer->str[buffer->len - 1])))
                g_string_append_c (buffer, PATH_SEP);

            g_string_append (buffer, element->vfs_prefix);
            g_string_append (buffer, VFS_PATH_URL_DELIMITER);

            url_str = vfs_path_build_url_params_str (element, !(flags & VPF_STRIP_PASSWORD));

            if (*url_str != '\0')
            {
                g_string_append (buffer, url_str);
                g_string_append_c (buffer, PATH_SEP);
            }

            g_free (url_str);
        }

#ifdef HAVE_CHARSET
        if ((flags & VPF_RECODE) == 0 && vfs_path_element_need_cleanup_converter (element))
        {
            if ((flags & VPF_HIDE_CHARSET) == 0)
            {
                if ((!is_relative)
                    && (buffer->len == 0 || !IS_PATH_SEP (buffer->str[buffer->len - 1])))
                    g_string_append (buffer, PATH_SEP_STR);
                g_string_append (buffer, VFS_ENCODING_PREFIX);
                g_string_append (buffer, element->encoding);
            }
            str_vfs_convert_from (element->dir.converter, element->path, recode_buffer);
            vfs_append_from_path (recode_buffer->str, is_relative);
            g_string_set_size (recode_buffer, 0);
        }
        else
#endif
        {
            vfs_append_from_path (element->path, is_relative);
        }
    }
    g_string_free (recode_buffer, TRUE);
    return g_string_free (buffer, FALSE);
}

#undef vfs_append_from_path

/* --------------------------------------------------------------------------------------------- */
/**
 * Convert first elements_count elements from vfs_path_t to string representation.
 *
 * @param vpath pointer to vfs_path_t object
 * @param elements_count count of first elements for convert
 *
 * @return pointer to newly created string.
 */

char *
vfs_path_to_str_elements_count (const vfs_path_t * vpath, int elements_count)
{
    return vfs_path_to_str_flags (vpath, elements_count, VPF_NONE);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Split path string to path elements with flags for change parce process.
 *
 * @param path_str VFS-path
 * @param flags flags for parser
 *
 * @return pointer to newly created vfs_path_t object with filled path elements array.
 */

vfs_path_t *
vfs_path_from_str_flags (const char *path_str, vfs_path_flag_t flags)
{
    vfs_path_t *vpath;
    char *path;

    if (path_str == NULL)
        return NULL;

    if ((flags & VPF_NO_CANON) == 0)
        path = vfs_canon (path_str);
    else
        path = g_strdup (path_str);

    if (path == NULL)
        return NULL;

    if ((flags & VPF_USE_DEPRECATED_PARSER) != 0 && vfs_path_is_str_path_deprecated (path))
        vpath = vfs_path_from_str_deprecated_parser (path);
    else
        vpath = vfs_path_from_str_uri_parser (path);

    vpath->str = vfs_path_to_str_flags (vpath, 0, flags);
    g_free (path);

    return vpath;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Split path string to path elements.
 *
 * @param path_str VFS-path
 *
 * @return pointer to newly created vfs_path_t object with filled path elements array.
 */

vfs_path_t *
vfs_path_from_str (const char *path_str)
{
    return vfs_path_from_str_flags (path_str, VPF_NONE);
}

/* --------------------------------------------------------------------------------------------- */
/*
 * Create new vfs_path_t object.
 *
 * @return pointer to newly created vfs_path_t object.
 */

vfs_path_t *
vfs_path_new (void)
{
    vfs_path_t *vpath;

    vpath = g_new0 (vfs_path_t, 1);
    vpath->path = g_array_new (FALSE, TRUE, sizeof (vfs_path_element_t *));

    return vpath;
}

/* --------------------------------------------------------------------------------------------- */
/*
 * Get count of path elements.
 *
 * @param vpath pointer to vfs_path_t object
 *
 * @return count of path elements.
 */

int
vfs_path_elements_count (const vfs_path_t * vpath)
{
    return (vpath != NULL && vpath->path != NULL) ? vpath->path->len : 0;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Add vfs_path_element_t object to end of list in vfs_path_t object
 * @param vpath pointer to vfs_path_t object
 * @param path_element pointer to vfs_path_element_t object
 */

void
vfs_path_add_element (vfs_path_t * vpath, const vfs_path_element_t * path_element)
{
    g_array_append_val (vpath->path, path_element);
    g_free (vpath->str);
    vpath->str = vfs_path_to_str_flags (vpath, 0, VPF_NONE);
}

/* --------------------------------------------------------------------------------------------- */
/*
 * Get one path element by index.
 *
 * @param vpath pointer to vfs_path_t object
 * @param element_index element index. May have negative value (in this case count was started at the end of list).
 *
 * @return path element.
 */

const vfs_path_element_t *
vfs_path_get_by_index (const vfs_path_t * vpath, int element_index)
{
    if (vpath == NULL)
        return NULL;

    if (element_index < 0)
        element_index += vfs_path_elements_count (vpath);

    if (element_index < 0)
        vfs_die ("vfs_path_get_by_index: incorrect index!");

    return g_array_index (vpath->path, vfs_path_element_t *, element_index);
}

/* --------------------------------------------------------------------------------------------- */
/*
 * Clone one path element
 *
 * @param element pointer to vfs_path_element_t object
 *
 * @return Newly allocated path element
 */

vfs_path_element_t *
vfs_path_element_clone (const vfs_path_element_t * element)
{
    vfs_path_element_t *new_element = g_new (vfs_path_element_t, 1);

    new_element->user = g_strdup (element->user);
    new_element->password = g_strdup (element->password);
    new_element->host = g_strdup (element->host);
    new_element->ipv6 = element->ipv6;
    new_element->port = element->port;
    new_element->path = g_strdup (element->path);
    new_element->class = element->class;
    new_element->vfs_prefix = g_strdup (element->vfs_prefix);
#ifdef HAVE_CHARSET
    new_element->encoding = g_strdup (element->encoding);
    if (vfs_path_element_need_cleanup_converter (element) && new_element->encoding != NULL)
        new_element->dir.converter = str_crt_conv_from (new_element->encoding);
    else
        new_element->dir.converter = element->dir.converter;
#endif
    new_element->dir.info = element->dir.info;

    return new_element;
}

/* --------------------------------------------------------------------------------------------- */
/*
 * Free one path element.
 *
 * @param element pointer to vfs_path_element_t object
 *
 */

void
vfs_path_element_free (vfs_path_element_t * element)
{
    if (element == NULL)
        return;

    g_free (element->user);
    g_free (element->password);
    g_free (element->host);
    g_free (element->path);
    g_free (element->vfs_prefix);

#ifdef HAVE_CHARSET
    g_free (element->encoding);

    if (vfs_path_element_need_cleanup_converter (element))
        str_close_conv (element->dir.converter);
#endif

    g_free (element);
}

/* --------------------------------------------------------------------------------------------- */
/*
 * Clone path
 *
 * @param vpath pointer to vfs_path_t object
 *
 * @return Newly allocated path object
 */

vfs_path_t *
vfs_path_clone (const vfs_path_t * vpath)
{
    vfs_path_t *new_vpath;
    int vpath_element_index;

    if (vpath == NULL)
        return NULL;

    new_vpath = vfs_path_new ();
    new_vpath->relative = vpath->relative;

    for (vpath_element_index = 0; vpath_element_index < vfs_path_elements_count (vpath);
         vpath_element_index++)
    {
        vfs_path_element_t *path_element;

        path_element = vfs_path_element_clone (vfs_path_get_by_index (vpath, vpath_element_index));
        g_array_append_val (new_vpath->path, path_element);
    }
    new_vpath->str = g_strdup (vpath->str);

    return new_vpath;
}

/* --------------------------------------------------------------------------------------------- */
/*
 * Free vfs_path_t object.
 *
 * @param vpath pointer to vfs_path_t object
 * @param free_str if TRUE the string representation of vpath is freed as well
 *
 * @return the string representation of vpath (i.e. NULL if free_str is TRUE)
 */

char *
vfs_path_free (vfs_path_t * vpath, gboolean free_str)
{
    int vpath_element_index;
    char *ret;

    if (vpath == NULL)
        return NULL;

    for (vpath_element_index = 0; vpath_element_index < vfs_path_elements_count (vpath);
         vpath_element_index++)
    {
        vfs_path_element_t *path_element;

        path_element = (vfs_path_element_t *) vfs_path_get_by_index (vpath, vpath_element_index);
        vfs_path_element_free (path_element);
    }

    g_array_free (vpath->path, TRUE);

    if (!free_str)
        ret = vpath->str;
    else
    {
        g_free (vpath->str);
        ret = NULL;
    }

    g_free (vpath);

    return ret;
}

/* --------------------------------------------------------------------------------------------- */
/*
 * Remove one path element by index
 *
 * @param vpath pointer to vfs_path_t object
 * @param element_index element index. May have negative value (in this case count was started at the end of list).
 *
 */

void
vfs_path_remove_element_by_index (vfs_path_t * vpath, int element_index)
{
    vfs_path_element_t *element;

    if ((vpath == NULL) || (vfs_path_elements_count (vpath) == 1))
        return;

    if (element_index < 0)
        element_index = vfs_path_elements_count (vpath) + element_index;

    element = (vfs_path_element_t *) vfs_path_get_by_index (vpath, element_index);
    vpath->path = g_array_remove_index (vpath->path, element_index);
    vfs_path_element_free (element);
    g_free (vpath->str);
    vpath->str = vfs_path_to_str_flags (vpath, 0, VPF_NONE);
}

/* --------------------------------------------------------------------------------------------- */
/** Return VFS class for the given prefix */

struct vfs_class *
vfs_prefix_to_class (const char *prefix)
{
    guint i;

    /* Avoid first class (localfs) that would accept any prefix */
    for (i = 1; i < vfs__classes_list->len; i++)
    {
        struct vfs_class *vfs;

        vfs = VFS_CLASS (g_ptr_array_index (vfs__classes_list, i));
        if (vfs->which != NULL)
        {
            if (vfs->which (vfs, prefix) == -1)
                continue;
            return vfs;
        }

        if (vfs->prefix != NULL && strncmp (prefix, vfs->prefix, strlen (vfs->prefix)) == 0)
            return vfs;
    }

    return NULL;
}

/* --------------------------------------------------------------------------------------------- */

#ifdef HAVE_CHARSET

/**
 * Check if need cleanup charset converter for vfs_path_element_t
 *
 * @param element part of path
 *
 * @return TRUE if need cleanup converter or FALSE otherwise
 */

gboolean
vfs_path_element_need_cleanup_converter (const vfs_path_element_t * element)
{
    return (element->dir.converter != str_cnv_from_term && element->dir.converter != INVALID_CONV);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Change encoding for last part (vfs_path_element_t) of vpath
 *
 * @param vpath pointer to path structure
 * encoding name of charset
 *
 * @return pointer to path structure (for use function in anoter functions)
 */
vfs_path_t *
vfs_path_change_encoding (vfs_path_t * vpath, const char *encoding)
{
    vfs_path_element_t *path_element;

    path_element = (vfs_path_element_t *) vfs_path_get_by_index (vpath, -1);
    /* don't add current encoding */
    if ((path_element->encoding != NULL) && (strcmp (encoding, path_element->encoding) == 0))
        return vpath;

    g_free (path_element->encoding);
    path_element->encoding = g_strdup (encoding);

    if (vfs_path_element_need_cleanup_converter (path_element))
        str_close_conv (path_element->dir.converter);

    path_element->dir.converter = str_crt_conv_from (path_element->encoding);

    g_free (vpath->str);
    vpath->str = vfs_path_to_str_flags (vpath, 0, VPF_NONE);
    return vpath;
}

#endif

/* --------------------------------------------------------------------------------------------- */

/**
 * Serialize vfs_path_t object to string
 *
 * @param vpath data for serialization
 * @param error contain pointer to object for handle error code and message
 *
 * @return serialized vpath as newly allocated string
 */

char *
vfs_path_serialize (const vfs_path_t * vpath, GError ** mcerror)
{
    mc_config_t *cpath;
    ssize_t element_index;
    char *ret_value;

    mc_return_val_if_error (mcerror, FALSE);

    if ((vpath == NULL) || (vfs_path_elements_count (vpath) == 0))
    {
        mc_propagate_error (mcerror, 0, "%s", "vpath object is empty");
        return NULL;
    }

    cpath = mc_config_init (NULL, FALSE);

    for (element_index = 0; element_index < vfs_path_elements_count (vpath); element_index++)
    {
        char groupname[BUF_TINY];
        const vfs_path_element_t *element;

        g_snprintf (groupname, sizeof (groupname), "path-element-%zd", element_index);
        element = vfs_path_get_by_index (vpath, element_index);
        /* convert one element to config group */

        mc_config_set_string_raw (cpath, groupname, "path", element->path);
        mc_config_set_string_raw (cpath, groupname, "class-name", element->class->name);
#ifdef HAVE_CHARSET
        mc_config_set_string_raw (cpath, groupname, "encoding", element->encoding);
#endif
        mc_config_set_string_raw (cpath, groupname, "vfs_prefix", element->vfs_prefix);

        mc_config_set_string_raw (cpath, groupname, "user", element->user);
        mc_config_set_string_raw (cpath, groupname, "password", element->password);
        mc_config_set_string_raw (cpath, groupname, "host", element->host);
        if (element->port != 0)
            mc_config_set_int (cpath, groupname, "port", element->port);
    }

    ret_value = mc_serialize_config (cpath, mcerror);
    mc_config_deinit (cpath);
    return ret_value;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Deserialize string to vfs_path_t object
 *
 * @param data data for serialization
 * @param error contain pointer to object for handle error code and message
 *
 * @return newly allocated vfs_path_t object
 */

vfs_path_t *
vfs_path_deserialize (const char *data, GError ** mcerror)
{
    mc_config_t *cpath;
    size_t element_index;
    vfs_path_t *vpath;

    mc_return_val_if_error (mcerror, FALSE);

    cpath = mc_deserialize_config (data, mcerror);
    if (cpath == NULL)
        return NULL;

    vpath = vfs_path_new ();

    for (element_index = 0;; element_index++)
    {
        struct vfs_class *eclass;
        vfs_path_element_t *element;
        char *cfg_value;
        char groupname[BUF_TINY];

        g_snprintf (groupname, sizeof (groupname), "path-element-%zu", element_index);
        if (!mc_config_has_group (cpath, groupname))
            break;

        cfg_value = mc_config_get_string_raw (cpath, groupname, "class-name", NULL);
        eclass = vfs_get_class_by_name (cfg_value);
        if (eclass == NULL)
        {
            vfs_path_free (vpath, TRUE);
            g_set_error (mcerror, MC_ERROR, 0, "Unable to find VFS class by name '%s'", cfg_value);
            g_free (cfg_value);
            mc_config_deinit (cpath);
            return NULL;
        }
        g_free (cfg_value);

        element = g_new0 (vfs_path_element_t, 1);
        element->class = eclass;
        element->path = mc_config_get_string_raw (cpath, groupname, "path", NULL);

#ifdef HAVE_CHARSET
        element->encoding = mc_config_get_string_raw (cpath, groupname, "encoding", NULL);
        element->dir.converter =
            (element->encoding != NULL) ? str_crt_conv_from (element->encoding) : INVALID_CONV;
#endif

        element->vfs_prefix = mc_config_get_string_raw (cpath, groupname, "vfs_prefix", NULL);

        element->user = mc_config_get_string_raw (cpath, groupname, "user", NULL);
        element->password = mc_config_get_string_raw (cpath, groupname, "password", NULL);
        element->host = mc_config_get_string_raw (cpath, groupname, "host", NULL);
        element->port = mc_config_get_int (cpath, groupname, "port", 0);

        vpath->path = g_array_append_val (vpath->path, element);
    }

    mc_config_deinit (cpath);
    if (vfs_path_elements_count (vpath) == 0)
    {
        vfs_path_free (vpath, TRUE);
        g_set_error (mcerror, MC_ERROR, 0, "No any path elements found");
        return NULL;
    }
    vpath->str = vfs_path_to_str_flags (vpath, 0, VPF_NONE);

    return vpath;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Build vfs_path_t object from arguments.
 *
 * @param first_element of path
 * @param ... path tokens, terminated by NULL
 *
 * @return newly allocated vfs_path_t object
 */

vfs_path_t *
vfs_path_build_filename (const char *first_element, ...)
{
    va_list args;
    char *str_path;
    vfs_path_t *vpath;

    if (first_element == NULL)
        return NULL;

    va_start (args, first_element);
    str_path = mc_build_filenamev (first_element, args);
    va_end (args);
    vpath = vfs_path_from_str (str_path);
    g_free (str_path);
    return vpath;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Append tokens to path object
 *
 * @param vpath path object
 * @param first_element of path
 * @param ... NULL-terminated strings
 *
 * @return newly allocated path object
 */

vfs_path_t *
vfs_path_append_new (const vfs_path_t * vpath, const char *first_element, ...)
{
    va_list args;
    char *str_path;
    const char *result_str;
    vfs_path_t *ret_vpath;

    if (vpath == NULL || first_element == NULL)
        return NULL;

    va_start (args, first_element);
    str_path = mc_build_filenamev (first_element, args);
    va_end (args);

    result_str = vfs_path_as_str (vpath);
    ret_vpath = vfs_path_build_filename (result_str, str_path, (char *) NULL);
    g_free (str_path);

    return ret_vpath;

}

/* --------------------------------------------------------------------------------------------- */

/**
 * Append vpath_t tokens to path object
 *
 * @param first_vpath vpath objects
 * @param ... NULL-terminated vpath objects
 *
 * @return newly allocated path object
 */

vfs_path_t *
vfs_path_append_vpath_new (const vfs_path_t * first_vpath, ...)
{
    va_list args;
    vfs_path_t *ret_vpath;
    const vfs_path_t *current_vpath = first_vpath;

    if (first_vpath == NULL)
        return NULL;

    ret_vpath = vfs_path_new ();

    va_start (args, first_vpath);
    do
    {
        int vindex;

        for (vindex = 0; vindex < vfs_path_elements_count (current_vpath); vindex++)
        {
            vfs_path_element_t *path_element;

            path_element = vfs_path_element_clone (vfs_path_get_by_index (current_vpath, vindex));
            g_array_append_val (ret_vpath->path, path_element);
        }
        current_vpath = va_arg (args, const vfs_path_t *);
    }
    while (current_vpath != NULL);
    va_end (args);

    ret_vpath->str = vfs_path_to_str_flags (ret_vpath, 0, VPF_NONE);

    return ret_vpath;
}

/* --------------------------------------------------------------------------------------------- */

/**
 * get tockens count in path.
 *
 * @param vpath path object
 *
 * @return count of tokens
 */

size_t
vfs_path_tokens_count (const vfs_path_t * vpath)
{
    size_t count_tokens = 0;
    int element_index;

    if (vpath == NULL)
        return 0;

    for (element_index = 0; element_index < vfs_path_elements_count (vpath); element_index++)
    {
        const vfs_path_element_t *element;
        const char *token, *prev_token;

        element = vfs_path_get_by_index (vpath, element_index);

        for (prev_token = element->path; (token = strchr (prev_token, PATH_SEP)) != NULL;
             prev_token = token + 1)
        {
            /* skip empty substring */
            if (token != prev_token)
                count_tokens++;
        }

        if (*prev_token != '\0')
            count_tokens++;
    }

    return count_tokens;
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Get subpath by tokens
 *
 * @param vpath path object
 * @param start_position first token for got/ Started from 0.
 *        If negative, then position will be relative to end of path
 * @param length count of tokens
 *
 * @return newly allocated string with path tokens separated by slash
 */

char *
vfs_path_tokens_get (const vfs_path_t * vpath, ssize_t start_position, ssize_t length)
{
    GString *ret_tokens, *element_tokens;
    int element_index;
    size_t tokens_count = vfs_path_tokens_count (vpath);

    if (vpath == NULL)
        return NULL;

    if (length == 0)
        length = tokens_count;

    if (length < 0)
        length = tokens_count + length;

    if (start_position < 0)
        start_position = (ssize_t) tokens_count + start_position;

    if (start_position < 0)
        return NULL;

    if (start_position >= (ssize_t) tokens_count)
        return NULL;

    if (start_position + (ssize_t) length > (ssize_t) tokens_count)
        length = tokens_count - start_position;

    ret_tokens = g_string_sized_new (32);
    element_tokens = g_string_sized_new (32);

    for (element_index = 0; element_index < vfs_path_elements_count (vpath); element_index++)
    {
        const vfs_path_element_t *element;
        char **path_tokens, **iterator;

        g_string_assign (element_tokens, "");
        element = vfs_path_get_by_index (vpath, element_index);
        path_tokens = g_strsplit (element->path, PATH_SEP_STR, -1);

        for (iterator = path_tokens; *iterator != NULL; iterator++)
        {
            if (**iterator != '\0')
            {
                if (start_position == 0)
                {
                    if (length == 0)
                    {
                        vfs_path_tokens_add_class_info (element, ret_tokens, element_tokens);
                        g_string_free (element_tokens, TRUE);
                        g_strfreev (path_tokens);
                        return g_string_free (ret_tokens, FALSE);
                    }
                    length--;
                    if (element_tokens->len != 0)
                        g_string_append_c (element_tokens, PATH_SEP);
                    g_string_append (element_tokens, *iterator);
                }
                else
                    start_position--;
            }
        }
        g_strfreev (path_tokens);
        vfs_path_tokens_add_class_info (element, ret_tokens, element_tokens);
    }

    g_string_free (element_tokens, TRUE);
    return g_string_free (ret_tokens, !(start_position == 0 && length == 0));
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Get subpath by tokens
 *
 * @param vpath path object
 * @param start_position first token for got/ Started from 0.
 *        If negative, then position will be relative to end of path
 * @param length count of tokens
 *
 * @return newly allocated path object with path tokens separated by slash
 */

vfs_path_t *
vfs_path_vtokens_get (const vfs_path_t * vpath, ssize_t start_position, ssize_t length)
{
    char *str_tokens;
    vfs_path_t *ret_vpath = NULL;

    str_tokens = vfs_path_tokens_get (vpath, start_position, length);
    if (str_tokens != NULL)
    {
        ret_vpath = vfs_path_from_str_flags (str_tokens, VPF_NO_CANON);
        g_free (str_tokens);
    }
    return ret_vpath;
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Build URL parameters (such as user:pass @ host:port) from one path element object
 *
 * @param element path element
 * @param keep_password TRUE or FALSE
 *
 * @return newly allocated string
 */

char *
vfs_path_build_url_params_str (const vfs_path_element_t * element, gboolean keep_password)
{
    GString *buffer;

    if (element == NULL)
        return NULL;

    buffer = g_string_new ("");

    if (element->user != NULL)
        g_string_append (buffer, element->user);

    if (element->password != NULL && keep_password)
    {
        g_string_append_c (buffer, ':');
        g_string_append (buffer, element->password);
    }

    if (element->host != NULL)
    {
        if ((element->user != NULL) || (element->password != NULL))
            g_string_append_c (buffer, '@');
        if (element->ipv6)
            g_string_append_c (buffer, '[');
        g_string_append (buffer, element->host);
        if (element->ipv6)
            g_string_append_c (buffer, ']');
    }

    if ((element->port) != 0 && (element->host != NULL))
    {
        g_string_append_c (buffer, ':');
        g_string_append_printf (buffer, "%d", element->port);
    }

    return g_string_free (buffer, FALSE);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Build pretty string representation of one path_element_t object
 *
 * @param element path element
 *
 * @return newly allocated string
 */

char *
vfs_path_element_build_pretty_path_str (const vfs_path_element_t * element)
{
    char *url_params;
    GString *pretty_path;

    pretty_path = g_string_new (element->class->prefix);
    g_string_append (pretty_path, VFS_PATH_URL_DELIMITER);

    url_params = vfs_path_build_url_params_str (element, FALSE);
    g_string_append (pretty_path, url_params);
    g_free (url_params);

    if (!IS_PATH_SEP (*element->path))
        g_string_append_c (pretty_path, PATH_SEP);

    g_string_append (pretty_path, element->path);
    return g_string_free (pretty_path, FALSE);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Compare two path objects as strings
 *
 * @param vpath1 first path object
 * @param vpath2 second vpath object
 *
 * @return integer value like to strcmp.
 */

gboolean
vfs_path_equal (const vfs_path_t * vpath1, const vfs_path_t * vpath2)
{
    const char *path1, *path2;
    gboolean ret_val;

    if (vpath1 == NULL || vpath2 == NULL)
        return FALSE;

    path1 = vfs_path_as_str (vpath1);
    path2 = vfs_path_as_str (vpath2);

    ret_val = strcmp (path1, path2) == 0;

    return ret_val;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Compare two path objects as strings
 *
 * @param vpath1 first path object
 * @param vpath2 second vpath object
 * @param len number of first 'len' characters
 *
 * @return integer value like to strcmp.
 */

gboolean
vfs_path_equal_len (const vfs_path_t * vpath1, const vfs_path_t * vpath2, size_t len)
{
    const char *path1, *path2;
    gboolean ret_val;

    if (vpath1 == NULL || vpath2 == NULL)
        return FALSE;

    path1 = vfs_path_as_str (vpath1);
    path2 = vfs_path_as_str (vpath2);

    ret_val = strncmp (path1, path2, len) == 0;

    return ret_val;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Calculate path length in string representation
 *
 * @param vpath path object
 *
 * @return length of path
 */

size_t
vfs_path_len (const vfs_path_t * vpath)
{
    if (vpath == NULL)
        return 0;

    return strlen (vpath->str);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Convert relative vpath object to absolute
 *
 * @param vpath path object
 *
 * @return absolute path object
 */

vfs_path_t *
vfs_path_to_absolute (const vfs_path_t * vpath)
{
    vfs_path_t *absolute_vpath;
    const char *path_str;

    if (!vpath->relative)
        return vfs_path_clone (vpath);

    path_str = vfs_path_as_str (vpath);
    absolute_vpath = vfs_path_from_str (path_str);
    return absolute_vpath;
}

/* --------------------------------------------------------------------------------------------- */
