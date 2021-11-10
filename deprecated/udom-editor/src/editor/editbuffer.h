/** \file
 *  \brief Header: text keep buffer for WEdit
 */

#ifndef MC__EDIT_BUFFER_H
#define MC__EDIT_BUFFER_H

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct edit_buffer_struct
{
    off_t curs1;                /* position of the cursor from the beginning of the file. */
    off_t curs2;                /* position from the end of the file */
    GPtrArray *b1;              /* all data up to curs1 */
    GPtrArray *b2;              /* all data from end of file down to curs2 */
    off_t size;                 /* file size */
    long lines;                 /* total lines in the file */
    long curs_line;             /* line number of the cursor. */
} edit_buffer_t;

typedef struct edit_buffer_read_file_status_msg_struct
{
    simple_status_msg_t status_msg;     /* base class */

    gboolean first;
    edit_buffer_t *buf;
    off_t loaded;
} edit_buffer_read_file_status_msg_t;

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

void edit_buffer_init (edit_buffer_t * buf, off_t size);
void edit_buffer_clean (edit_buffer_t * buf);

int edit_buffer_get_byte (const edit_buffer_t * buf, off_t byte_index);
#ifdef HAVE_CHARSET
int edit_buffer_get_utf (const edit_buffer_t * buf, off_t byte_index, int *char_length);
int edit_buffer_get_prev_utf (const edit_buffer_t * buf, off_t byte_index, int *char_length);
#endif
long edit_buffer_count_lines (const edit_buffer_t * buf, off_t first, off_t last);
off_t edit_buffer_get_bol (const edit_buffer_t * buf, off_t current);
off_t edit_buffer_get_eol (const edit_buffer_t * buf, off_t current);
GString *edit_buffer_get_word_from_pos (const edit_buffer_t * buf, off_t start_pos, off_t * start,
                                        gsize * cut);
gboolean edit_buffer_find_word_start (const edit_buffer_t * buf, off_t * word_start,
                                      gsize * word_len);

void edit_buffer_insert (edit_buffer_t * buf, int c);
void edit_buffer_insert_ahead (edit_buffer_t * buf, int c);
int edit_buffer_delete (edit_buffer_t * buf);
int edit_buffer_backspace (edit_buffer_t * buf);

off_t edit_buffer_get_forward_offset (const edit_buffer_t * buf, off_t current, long lines,
                                      off_t upto);
off_t edit_buffer_get_backward_offset (const edit_buffer_t * buf, off_t current, long lines);

off_t edit_buffer_read_file (edit_buffer_t * buf, int fd, off_t size,
                             edit_buffer_read_file_status_msg_t * sm, gboolean * aborted);
off_t edit_buffer_write_file (edit_buffer_t * buf, int fd);

int edit_buffer_calc_percent (const edit_buffer_t * buf, off_t offset);

/*** inline functions ****************************************************************************/

static inline int
edit_buffer_get_current_byte (const edit_buffer_t * buf)
{
    return edit_buffer_get_byte (buf, buf->curs1);
}

/* --------------------------------------------------------------------------------------------- */

static inline int
edit_buffer_get_previous_byte (const edit_buffer_t * buf)
{
    return edit_buffer_get_byte (buf, buf->curs1 - 1);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Get "begin-of-line" offset of current line
 *
 * @param buf editor buffer
 *
 * @return index of first char of current line
 */

static inline off_t
edit_buffer_get_current_bol (const edit_buffer_t * buf)
{
    return edit_buffer_get_bol (buf, buf->curs1);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Get "end-of-line" offset of current line
 *
 * @param buf editor buffer
 *
 * @return index of first char of current line + 1
 */

static inline off_t
edit_buffer_get_current_eol (const edit_buffer_t * buf)
{
    return edit_buffer_get_eol (buf, buf->curs1);
}

/* --------------------------------------------------------------------------------------------- */

#endif /* MC__EDIT_BUFFER_H */
