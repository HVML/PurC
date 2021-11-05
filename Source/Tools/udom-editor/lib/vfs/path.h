#ifndef MC__VFS_PATH_H
#define MC__VFS_PATH_H

/*** typedefs(not structures) and defined constants **********************************************/

#define VFS_PATH_URL_DELIMITER "://"

/*** enums ***************************************************************************************/

typedef enum
{
    VPF_NONE = 0,
    VPF_NO_CANON = 1 << 0,
    VPF_USE_DEPRECATED_PARSER = 1 << 1,
    VPF_RECODE = 1 << 2,
    VPF_STRIP_HOME = 1 << 3,
    VPF_STRIP_PASSWORD = 1 << 4,
    VPF_HIDE_CHARSET = 1 << 5
} vfs_path_flag_t;

/*** structures declarations (and typedefs of structures)*****************************************/

struct vfs_class;
struct vfs_url_struct;

typedef struct
{
    gboolean relative;
    GArray *path;
    char *str;
} vfs_path_t;

typedef struct
{
    char *user;
    char *password;
    char *host;
    gboolean ipv6;
    int port;
    char *path;
    struct vfs_class *class;
#ifdef HAVE_CHARSET
    char *encoding;
#endif
    char *vfs_prefix;

    struct
    {
#ifdef HAVE_CHARSET
        GIConv converter;
#endif
        DIR *info;
    } dir;
} vfs_path_element_t;

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

vfs_path_t *vfs_path_new (void);
vfs_path_t *vfs_path_clone (const vfs_path_t * vpath);
void vfs_path_remove_element_by_index (vfs_path_t * vpath, int element_index);
char *vfs_path_free (vfs_path_t * path, gboolean free_str);
int vfs_path_elements_count (const vfs_path_t * path);

char *vfs_path_to_str_elements_count (const vfs_path_t * path, int elements_count);
char *vfs_path_to_str_flags (const vfs_path_t * vpath, int elements_count, vfs_path_flag_t flags);
vfs_path_t *vfs_path_from_str (const char *path_str);
vfs_path_t *vfs_path_from_str_flags (const char *path_str, vfs_path_flag_t flags);
vfs_path_t *vfs_path_build_filename (const char *first_element, ...);
vfs_path_t *vfs_path_append_new (const vfs_path_t * vpath, const char *first_element, ...);
vfs_path_t *vfs_path_append_vpath_new (const vfs_path_t * first_vpath, ...);
size_t vfs_path_tokens_count (const vfs_path_t * vpath);
char *vfs_path_tokens_get (const vfs_path_t * vpath, ssize_t start_position, ssize_t length);
vfs_path_t *vfs_path_vtokens_get (const vfs_path_t * vpath, ssize_t start_position, ssize_t length);

void vfs_path_add_element (vfs_path_t * vpath, const vfs_path_element_t * path_element);
const vfs_path_element_t *vfs_path_get_by_index (const vfs_path_t * path, int element_index);
vfs_path_element_t *vfs_path_element_clone (const vfs_path_element_t * element);
void vfs_path_element_free (vfs_path_element_t * element);

struct vfs_class *vfs_prefix_to_class (const char *prefix);

#ifdef HAVE_CHARSET
gboolean vfs_path_element_need_cleanup_converter (const vfs_path_element_t * element);
vfs_path_t *vfs_path_change_encoding (vfs_path_t * vpath, const char *encoding);
#endif

char *vfs_path_serialize (const vfs_path_t * vpath, GError ** error);
vfs_path_t *vfs_path_deserialize (const char *data, GError ** error);

char *vfs_path_build_url_params_str (const vfs_path_element_t * element, gboolean keep_password);
char *vfs_path_element_build_pretty_path_str (const vfs_path_element_t * element);

size_t vfs_path_len (const vfs_path_t * vpath);
gboolean vfs_path_equal (const vfs_path_t * vpath1, const vfs_path_t * vpath2);
gboolean vfs_path_equal_len (const vfs_path_t * vpath1, const vfs_path_t * vpath2, size_t len);
vfs_path_t *vfs_path_to_absolute (const vfs_path_t * vpath);

/*** inline functions ****************************************************************************/

static inline gboolean
vfs_path_element_valid (const vfs_path_element_t * element)
{
    return (element != NULL && element->class != NULL);
}

/* --------------------------------------------------------------------------------------------- */

static inline const char *
vfs_path_get_last_path_str (const vfs_path_t * vpath)
{
    const vfs_path_element_t *element;
    if (vpath == NULL)
        return NULL;
    element = vfs_path_get_by_index (vpath, -1);
    return (element != NULL) ? element->path : NULL;
}

/* --------------------------------------------------------------------------------------------- */

static inline const struct vfs_class *
vfs_path_get_last_path_vfs (const vfs_path_t * vpath)
{
    const vfs_path_element_t *element;
    if (vpath == NULL)
        return NULL;
    element = vfs_path_get_by_index (vpath, -1);
    return (element != NULL) ? element->class : NULL;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Convert vfs_path_t to string representation.
 *
 * @param vpath pointer to vfs_path_t object
 *
 * @return pointer to constant string
 */

static inline const char *
vfs_path_as_str (const vfs_path_t * vpath)
{
    return (vpath == NULL ? NULL : vpath->str);
}

/* --------------------------------------------------------------------------------------------- */

#endif
