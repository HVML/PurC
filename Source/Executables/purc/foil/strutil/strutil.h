#ifndef MC_STRUTIL_H
#define MC_STRUTIL_H

#include "../screen.h"

#include <sys/types.h>
#include <inttypes.h>
#include <string.h>

/* Header file for strutil.c, strutilascii.c, strutil8bit.c, strutilutf8.c.
 * There are two sort of functions:
 * 1. functions for working with growing strings and conversion strings between
 *    different encodings.
 *    (implemented directly in strutil.c)
 * 2. functions, that hide differences between encodings derived from ASCII.
 *    (implemented separately in strutilascii.c, strutil8bit.c, strutilutf8.c)
 * documentation is made for UTF-8 version of functions.
 */

/* invalid strings
 * function, that works with invalid strings are marked with "I" 
 * in documentation
 * invalid bytes of string are handled as one byte characters with width 1, they
 * are displayed as questionmarks, I-maked comparing functions try to keep 
 * the original value of these bytes.
 */

/* combining characters
 * displaynig: all handled as zero with characters, expect combing character 
 * at the begin of string, this character has with one (space add before), 
 * so str_term_width is not good for computing width of singles characters 
 * (never return zero, expect emtpy string)
 * for compatibility are strings composed before displaynig
 * comparing: comparing decompose all string before comparing, n-compare 
 * functions do not work as is usual, because same strings do not have to be 
 * same length in UTF-8. So they return 0 if one string is prefix of the other 
 * one. 
 * str_prefix is used to determine, how many characters from one string are 
 * prefix in second string. However, str_prefix return number of characters in
 * decompose form. (used in do_search (screen.c))
 */

/*** typedefs(not structures) and defined constants **********************************************/

#define IS_FIT(x) ((x) & 0x0010)
#define MAKE_FIT(x) ((x) | 0x0010)
#define HIDE_FIT(x) ((x) & 0x000f)

#define INVALID_CONV ((GIConv) (-1))

/*** enums ***************************************************************************************/

/* results of conversion function
 */
typedef enum
{
    /* success means, that convertion has been finished successully
     */
    ESTR_SUCCESS = 0,
    /* problem means, that not every characters was successfully converted (They are
     * replaced with questionmark). So is impossible convert string back. 
     */
    ESTR_PROBLEM = 1,
    /* failure means, that conversion is not possible (example: wrong encoding 
     * of input string)
     */
    ESTR_FAILURE = 2
} estr_t;

/* alignment strings on terminal
 */
typedef enum
{
    J_LEFT = 0x01,
    J_RIGHT = 0x02,
    J_CENTER = 0x03,
    /* if there is enough space for string on terminal,
     * string is centered otherwise is aligned to left */
    J_CENTER_LEFT = 0x04,
    /* fit alignment, if string is to long, is truncated with '~' */
    J_LEFT_FIT = 0x11,
    J_RIGHT_FIT = 0x12,
    J_CENTER_FIT = 0x13,
    J_CENTER_LEFT_FIT = 0x14
} align_crt_t;

/* string-to-integer parsing results
 */
typedef enum
{
    LONGINT_OK = 0,

    /* These two values can be ORed together, to indicate that both errors occurred. */
    LONGINT_OVERFLOW = 1,
    LONGINT_INVALID_SUFFIX_CHAR = 2,

    LONGINT_INVALID_SUFFIX_CHAR_WITH_OVERFLOW = (LONGINT_INVALID_SUFFIX_CHAR | LONGINT_OVERFLOW),
    LONGINT_INVALID = 4
} strtol_error_t;

/*** structures declarations (and typedefs of structures)*****************************************/

/* all functions in str_class must be defined for every encoding */
struct str_class
{
    /* *INDENT-OFF* */
    gchar *(*conv_gerror_message) (GError * error, const char *def_msg);
      /*I*/ estr_t (*vfs_convert_to) (GIConv coder, const char *string, int size, GString * buffer);
      /*I*/ void (*insert_replace_char) (GString * buffer);
    gboolean (*is_valid_string) (const char *text);
      /*I*/ int (*is_valid_char) (const char *ch, size_t size);
      /*I*/ void (*cnext_char) (const char **text);
    void (*cprev_char) (const char **text);
    void (*cnext_char_safe) (const char **text);
      /*I*/ void (*cprev_char_safe) (const char **text);
      /*I*/ int (*cnext_noncomb_char) (const char **text);
      /*I*/ int (*cprev_noncomb_char) (const char **text, const char *begin);
      /*I*/ gboolean (*char_isspace) (const char *ch);
      /*I*/ gboolean (*char_ispunct) (const char *ch);
      /*I*/ gboolean (*char_isalnum) (const char *ch);
      /*I*/ gboolean (*char_isdigit) (const char *ch);
      /*I*/ gboolean (*char_isprint) (const char *ch);
      /*I*/ gboolean (*char_iscombiningmark) (const char *ch);
      /*I*/ int (*length) (const char *text);
      /*I*/ int (*length2) (const char *text, int size);
      /*I*/ int (*length_noncomb) (const char *text);
      /*I*/ gboolean (*char_toupper) (const char *ch, char **out, size_t * remain);
    gboolean (*char_tolower) (const char *ch, char **out, size_t * remain);
    void (*fix_string) (char *text);
      /*I*/ const char *(*term_form) (const char *text);
      /*I*/ const char *(*fit_to_term) (const char *text, int width, align_crt_t just_mode);
      /*I*/ const char *(*term_trim) (const char *text, int width);
      /*I*/ const char *(*term_substring) (const char *text, int start, int width);
      /*I*/ int (*term_width1) (const char *text);
      /*I*/ int (*term_width2) (const char *text, size_t length);
      /*I*/ int (*term_char_width) (const char *length);
      /*I*/ const char *(*trunc) (const char *length, int width);
      /*I*/ int (*offset_to_pos) (const char *text, size_t length);
      /*I*/ int (*column_to_pos) (const char *text, size_t pos);
      /*I*/ char *(*create_search_needle) (const char *needle, gboolean case_sen);
    void (*release_search_needle) (char *needle, gboolean case_sen);
    const char *(*search_first) (const char *text, const char *needle, gboolean case_sen);
    const char *(*search_last) (const char *text, const char *needle, gboolean case_sen);
    int (*compare) (const char *t1, const char *t2);
      /*I*/ int (*ncompare) (const char *t1, const char *t2);
      /*I*/ int (*casecmp) (const char *t1, const char *t2);
      /*I*/ int (*ncasecmp) (const char *t1, const char *t2);
      /*I*/ int (*prefix) (const char *text, const char *prefix);
      /*I*/ int (*caseprefix) (const char *text, const char *prefix);
      /*I*/ char *(*create_key) (const char *text, gboolean case_sen);
      /*I*/ char *(*create_key_for_filename) (const char *text, gboolean case_sen);
      /*I*/ int (*key_collate) (const char *t1, const char *t2, gboolean case_sen);
      /*I*/ void (*release_key) (char *key, gboolean case_sen);
    /* *INDENT-ON* */
};

/*** global variables defined in .c file *********************************************************/

/* standard convertors */
extern GIConv str_cnv_to_term;
extern GIConv str_cnv_from_term;
/* from terminal encoding to terminal encoding */
extern GIConv str_cnv_not_convert;

/*** declarations of public functions ************************************************************/

struct str_class str_utf8_init (void);
struct str_class str_8bit_init (void);
struct str_class str_ascii_init (void);

/* create convertor from "from_enc" to terminal encoding
 * if "from_enc" is not supported return INVALID_CONV 
 */
GIConv str_crt_conv_from (const char *from_enc);

/* create convertor from terminal encoding to "to_enc"
 * if "to_enc" is not supported return INVALID_CONV 
 */
GIConv str_crt_conv_to (const char *to_enc);

/* close convertor, do not close str_cnv_to_term, str_cnv_from_term, 
 * str_cnv_not_convert 
 */
void str_close_conv (GIConv conv);

/* return on of not used buffers (.used == 0) or create new
 * returned buffer has set .used to 1
 */

/* convert string using coder, result of conversion is appended at end of buffer
 * return ESTR_SUCCESS if there was no problem.
 * otherwise return  ESTR_PROBLEM or ESTR_FAILURE
 */
estr_t str_convert (GIConv coder, const char *string, GString * buffer);
estr_t str_nconvert (GIConv coder, const char *string, int size, GString * buffer);

/* convert GError message (which in UTF-8) to terminal charset
 * def_char is used if result of error->str conversion if ESTR_FAILURE
 * return new allocated null-terminated string, which is need to be freed
 * I
 */
gchar *str_conv_gerror_message (GError * error, const char *def_msg);

/* return only ESTR_SUCCESS or ESTR_FAILURE, because vfs must be able to convert
 * result to original string. (so no replace with questionmark)
 * if coder is str_cnv_from_term or str_cnv_not_convert, string is only copied,
 * so is possible to show file, that is not valid in terminal encoding
 */
estr_t str_vfs_convert_from (GIConv coder, const char *string, GString * buffer);

/* if coder is str_cnv_to_term or str_cnv_not_convert, string is only copied,
 * does replace with questionmark 
 * I
 */
estr_t str_vfs_convert_to (GIConv coder, const char *string, int size, GString * buffer);

/* printf function for str_buffer, append result of printf at the end of buffer
 */
/* *INDENT-OFF* */
void str_printf (GString * buffer, const char *format, ...) G_GNUC_PRINTF (2, 3);
/* *INDENT-ON* */

/* add standard replacement character in terminal encoding
 */
void str_insert_replace_char (GString * buffer);

/* init strings and set terminal encoding,
 * if is termenc NULL, detect terminal encoding
 * create all str_cnv_* and set functions for terminal encoding
 */
void str_init_strings (const char *termenc);

/* free all str_buffer and all str_cnv_*
 */
void str_uninit_strings (void);

/* try convert characters in ch to output using conv
 * ch_size is size of ch, can by (size_t)(-1) (-1 only for ASCII 
 *     compatible encoding, for other must be set)
 * return ESTR_SUCCESS if conversion was successfully,
 * ESTR_PROBLEM if ch contains only part of characters,
 * ESTR_FAILURE if conversion is not possible
 */
estr_t str_translate_char (GIConv conv, const char *ch, size_t ch_size,
                           char *output, size_t out_size);

/* test, if text is valid in terminal encoding
 * I
 */
gboolean str_is_valid_string (const char *text);

/* test, if first char of ch is valid
 * size, how many bytes characters occupied, could be (size_t)(-1)
 * return 1 if it is valid, -1 if it is invalid or -2 if it is only part of 
 * multibyte character 
 * I
 */
int str_is_valid_char (const char *ch, size_t size);

/* return next characters after text, do not call on the end of string
 */
char *str_get_next_char (char *text);
const char *str_cget_next_char (const char *text);

/* return previous characters before text, do not call on the start of strings
 */
char *str_get_prev_char (char *text);
const char *str_cget_prev_char (const char *text);

/* set text to next characters, do not call on the end of string
 */
void str_next_char (char **text);
void str_cnext_char (const char **text);

/* set text to previous characters, do not call on the start of strings
 */
void str_prev_char (char **text);
void str_cprev_char (const char **text);

/* return next characters after text, do not call on the end of string
 * works with invalid string 
 * I
 */
char *str_get_next_char_safe (char *text);
const char *str_cget_next_char_safe (const char *text);

/* return previous characters before text, do not call on the start of strings
 * works with invalid string 
 * I
 */
char *str_get_prev_char_safe (char *text);
const char *str_cget_prev_char_safe (const char *text);

/* set text to next characters, do not call on the end of string
 * works with invalid string 
 * I
 */
void str_next_char_safe (char **text);
void str_cnext_char_safe (const char **text);

/* set text to previous characters, do not call on the start of strings
 * works with invalid string 
 * I
 */
void str_prev_char_safe (char **text);
void str_cprev_char_safe (const char **text);

/* set text to next noncombining characters, check the end of text
 * return how many characters was skipped
 * works with invalid string 
 * I
 */
int str_next_noncomb_char (char **text);
int str_cnext_noncomb_char (const char **text);

/* set text to previous noncombining characters, search stop at begin 
 * return how many characters was skipped
 * works with invalid string 
 * I
 */
int str_prev_noncomb_char (char **text, const char *begin);
int str_cprev_noncomb_char (const char **text, const char *begin);

/* if first characters in ch is space, tabulator  or new lines
 * I
 */
gboolean str_isspace (const char *ch);

/* if first characters in ch is punctuation or symbol
 * I
 */
gboolean str_ispunct (const char *ch);

/* if first characters in ch is alphanum
 * I
 */
gboolean str_isalnum (const char *ch);

/* if first characters in ch is digit
 * I
 */
gboolean str_isdigit (const char *ch);

/* if first characters in ch is printable
 * I
 */
gboolean str_isprint (const char *ch);

/* if first characters in ch is a combining mark (only in utf-8)
 * combining makrs are assumed to be zero width 
 * I
 */
gboolean str_iscombiningmark (const char *ch);

/* write lower from of fisrt characters in ch into out
 * decrase remain by size of returned characters
 * if out is not big enough, do nothing
 */
gboolean str_toupper (const char *ch, char **out, size_t * remain);

/* write upper from of fisrt characters in ch into out
 * decrase remain by size of returned characters
 * if out is not big enough, do nothing
 */
gboolean str_tolower (const char *ch, char **out, size_t * remain);

/* return length of text in characters
 * I
 */
int str_length (const char *text);

/* return length of text in characters, limit to size
 * I
 */
int str_length2 (const char *text, int size);

/* return length of one char
 * I
 */
int str_length_char (const char *text);

/* return length of text in characters, count only noncombining characters
 * I
 */
int str_length_noncomb (const char *text);

/* replace all invalid characters in text with questionmark
 * after return, text is valid string in terminal encoding
 * I
 */
void str_fix_string (char *text);

/* replace all invalid characters in text with questionmark
 * replace all unprintable characters with '.'
 * return static allocated string, "text" is not changed
 * returned string do not need to be freed
 * I
 */
const char *str_term_form (const char *text);

/* like str_term_form, but text can be alignment to width
 * alignment is specified in just_mode (J_LEFT, J_LEFT_FIT, ...)
 * result is completed with spaces to width
 * I
 */
const char *str_fit_to_term (const char *text, int width, align_crt_t just_mode);

/* like str_term_form, but when text is wider than width, three dots are
 * inserted at begin and result is completed with suffix of text
 * no additional spaces are inserted
 * I
 */
const char *str_term_trim (const char *text, int width);


/* like str_term_form, but return only specified substring
 * start - column (position) on terminal, where substring begin
 * result is completed with spaces to width
 * I
 */
const char *str_term_substring (const char *text, int start, int width);

/* return width, that will be text occupied on terminal
 * I
 */
int str_term_width1 (const char *text);

/* return width, that will be text occupied on terminal
 * text is limited by length in characters
 * I
 */
int str_term_width2 (const char *text, size_t length);

/* return width, that will be character occupied on terminal
 * combining characters are always zero width
 * I
 */
int str_term_char_width (const char *text);

/* convert position in characters to position in bytes 
 * I
 */
int str_offset_to_pos (const char *text, size_t length);

/* convert position on terminal to position in characters
 * I
 */
int str_column_to_pos (const char *text, size_t pos);

/* like str_fit_to_term width just_mode = J_LEFT_FIT, 
 * but do not insert additional spaces
 * I
 */
const char *str_trunc (const char *text, int width);

/* create needle, that will be searched in str_search_fist/last,
 * so needle can be reused
 * in UTF-8 return normalized form of needle
 */
char *str_create_search_needle (const char *needle, gboolean case_sen);

/* free needle returned by str_create_search_needle
 */
void str_release_search_needle (char *needle, gboolean case_sen);

/* search for first occurrence of search in text
 */
const char *str_search_first (const char *text, const char *needle, gboolean case_sen);

/* search for last occurrence of search in text
 */
const char *str_search_last (const char *text, const char *needle, gboolean case_sen);

/* case sensitive compare two strings
 * I
 */
int str_compare (const char *t1, const char *t2);

/* case sensitive compare two strings
 * if one string is prefix of the other string, return 0
 * I
 */
int str_ncompare (const char *t1, const char *t2);

/* case insensitive compare two strings
 * I
 */
int str_casecmp (const char *t1, const char *t2);

/* case insensitive compare two strings
 * if one string is prefix of the other string, return 0
 * I
 */
int str_ncasecmp (const char *t1, const char *t2);

/* return, how many bytes are are same from start in text and prefix
 * both strings are decomposed before comparing and return value is counted
 * in decomposed form, too. calling with prefix, prefix, you get size in bytes
 * of prefix in decomposed form,
 * I
 */
int str_prefix (const char *text, const char *prefix);

/* case insensitive version of str_prefix
 * I
 */
int str_caseprefix (const char *text, const char *prefix);

/* create a key that is used by str_key_collate
 * I
 */
char *str_create_key (const char *text, gboolean case_sen);

/* create a key that is used by str_key_collate
 * should aware dot '.' in text
 * I
 */
char *str_create_key_for_filename (const char *text, gboolean case_sen);

/* compare two string using LC_COLLATE, if is possible
 * if case_sen is set, comparing is case sensitive,
 * case_sen must be same for str_create_key, str_key_collate and str_release_key
 * I
 */
int str_key_collate (const char *t1, const char *t2, gboolean case_sen);

/* release_key created by str_create_key, only rigth way to release key
 * I
 */
void str_release_key (char *key, gboolean case_sen);

/* return TRUE if codeset_name is utf8 or utf-8
 * I
 */
gboolean str_isutf8 (const char *codeset_name);

const char *str_detect_termencoding (void);

int str_verscmp (const char *s1, const char *s2);

/* Compare version strings:

   This function compares strings s1 and s2:
   1) By PREFIX in the same way as strcmp.
   2) Then by VERSION (most similarly to version compare of Debian's dpkg).
   Leading zeros in version numbers are ignored.
   3) If both (PREFIX and  VERSION) are equal, strcmp function is used for
   comparison. So this function can return 0 if (and only if) strings s1
   and s2 are identical.

   It returns number > 0 for s1 > s2, 0 for s1 == s2 and number < 0 for s1 < s2.

   This function compares strings, in a way that if VER1 and VER2 are version
   numbers and PREFIX and SUFFIX (SUFFIX defined as (\.[A-Za-z~][A-Za-z0-9~]*)*)
   are strings then VER1 < VER2 implies filevercmp (PREFIX VER1 SUFFIX,
   PREFIX VER2 SUFFIX) < 0.

   This function is intended to be a replacement for strverscmp.
 */
int filevercmp (const char *s1, const char *s2);


/* return how many lines and columns will text occupy on terminal
 */
void str_msg_term_size (const char *text, int *lines, int *columns);

/**
 * skip first needle's in haystack
 *
 * @param haystack pointer to string
 * @param needle pointer to string
 * @param skip_count skip first bytes
 *
 * @return pointer to skip_count+1 needle (or NULL if not found).
 */

char *strrstr_skip_count (const char *haystack, const char *needle, size_t skip_count);

char *str_replace_all (const char *haystack, const char *needle, const char *replacement);

strtol_error_t xstrtoumax (const char *s, char **ptr, int base, uintmax_t * val,
                           const char *valid_suffixes);
uintmax_t parse_integer (const char *str, gboolean * invalid);

/* --------------------------------------------------------------------------------------------- */
/*** inline functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

static inline void
str_replace (char *s, char from, char to)
{
    for (; *s != '\0'; s++)
    {
        if (*s == from)
            *s = to;
    }
}

/* --------------------------------------------------------------------------------------------- */
/*
 * strcpy is unsafe on overlapping memory areas, so define memmove-alike
 * string function.
 * Have sense only when:
 *  * dest <= src
 *   AND
 *  * dest and str are pointers to one object (as Roland Illig pointed).
 *
 * We can't use str*cpy funs here:
 * http://kerneltrap.org/mailarchive/openbsd-misc/2008/5/27/1951294
 *
 * @param dest pointer to string
 * @param src pointer to string
 *
 * @return newly allocated string
 *
 */

static inline char *
str_move (char *dest, const char *src)
{
    size_t n;

    g_assert (dest <= src);

    n = strlen (src) + 1;       /* + '\0' */

    return (char *) memmove (dest, src, n);
}

char *strutils_escape (const char *src, gsize src_len, const char *escaped_chars,
                       gboolean escape_non_printable);
char *strutils_unescape (const char *src, gsize src_len, const char *unescaped_chars,
                         gboolean unescape_non_printable);
char *strutils_shell_unescape (const char *text);
char *strutils_shell_escape (const char *text);

char *strutils_glob_escape (const char *text);
char *strutils_glob_unescape (const char *text);

char *strutils_regex_escape (const char *text);
char *strutils_regex_unescape (const char *text);

gboolean strutils_is_char_escaped (const char *start, const char *current);

/* --------------------------------------------------------------------------------------------- */

#endif /* MC_STRUTIL_H */
