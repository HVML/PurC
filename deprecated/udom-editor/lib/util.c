/*
   Various utilities

   Copyright (C) 1994-2021
   Free Software Foundation, Inc.

   Written by:
   Miguel de Icaza, 1994, 1995, 1996
   Janne Kukonlehto, 1994, 1995, 1996
   Dugan Porter, 1994, 1995, 1996
   Jakub Jelinek, 1994, 1995, 1996
   Mauricio Plaza, 1994, 1995, 1996
   Slava Zanko <slavazanko@gmail.com>, 2013

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

/** \file lib/util.c
 *  \brief Source: various utilities
 */

#include <config.h>
#include "ue-features.h"

#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lib/global.h"
#include "lib/mcconfig.h"
#include "lib/fileloc.h"
#include "lib/vfs/vfs.h"
#include "lib/strutil.h"
#include "lib/util.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

#define ismode(n,m) ((n & m) == m)

/* Number of attempts to create a temporary file */
#ifndef TMP_MAX
#define TMP_MAX 16384
#endif /* !TMP_MAX */

#define TMP_SUFFIX ".tmp"

#define ASCII_A (0x40 + 1)
#define ASCII_Z (0x40 + 26)
#define ASCII_a (0x60 + 1)
#define ASCII_z (0x60 + 26)

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** file scope functions ************************************************************************/
/* --------------------------------------------------------------------------------------------- */

#ifndef HAVE_CHARSET
static inline int
is_7bit_printable (unsigned char c)
{
    return (c > 31 && c < 127);
}
#endif

/* --------------------------------------------------------------------------------------------- */

static inline int
is_iso_printable (unsigned char c)
{
    return ((c > 31 && c < 127) || c >= 160);
}

/* --------------------------------------------------------------------------------------------- */

static inline int
is_8bit_printable (unsigned char c)
{
    /* "Full 8 bits output" doesn't work on xterm */
    if (mc_global.tty.xterm_flag)
        return is_iso_printable (c);

    return (c > 31 && c != 127 && c != 155);
}

/* --------------------------------------------------------------------------------------------- */

static char *
resolve_symlinks (const vfs_path_t * vpath)
{
    char *p, *p2;
    char *buf, *buf2, *q, *r, c;
    struct stat mybuf;

    if (vpath->relative)
        return NULL;

    p = p2 = g_strdup (vfs_path_as_str (vpath));
    r = buf = g_malloc (MC_MAXPATHLEN);
    buf2 = g_malloc (MC_MAXPATHLEN);
    *r++ = PATH_SEP;
    *r = '\0';

    do
    {
        q = strchr (p + 1, PATH_SEP);
        if (q == NULL)
        {
            q = strchr (p + 1, '\0');
            if (q == p + 1)
                break;
        }
        c = *q;
        *q = '\0';
        if (mc_lstat (vpath, &mybuf) < 0)
        {
            MC_PTR_FREE (buf);
            goto ret;
        }
        if (!S_ISLNK (mybuf.st_mode))
            strcpy (r, p + 1);
        else
        {
            int len;

            len = mc_readlink (vpath, buf2, MC_MAXPATHLEN - 1);
            if (len < 0)
            {
                MC_PTR_FREE (buf);
                goto ret;
            }
            buf2[len] = '\0';
            if (IS_PATH_SEP (*buf2))
                strcpy (buf, buf2);
            else
                strcpy (r, buf2);
        }
        canonicalize_pathname (buf);
        r = strchr (buf, '\0');
        if (*r == '\0' || !IS_PATH_SEP (r[-1]))
            /* FIXME: this condition is always true because r points to the EOL */
        {
            *r++ = PATH_SEP;
            *r = '\0';
        }
        *q = c;
        p = q;
    }
    while (c != '\0');

    if (*buf == '\0')
        strcpy (buf, PATH_SEP_STR);
    else if (IS_PATH_SEP (r[-1]) && r != buf + 1)
        r[-1] = '\0';

  ret:
    g_free (buf2);
    g_free (p2);
    return buf;
}

/* --------------------------------------------------------------------------------------------- */

static gboolean
mc_util_write_backup_content (const char *from_file_name, const char *to_file_name)
{
    FILE *backup_fd;
    char *contents;
    gsize length;
    gboolean ret1 = TRUE;

    if (!g_file_get_contents (from_file_name, &contents, &length, NULL))
        return FALSE;

    backup_fd = fopen (to_file_name, "w");
    if (backup_fd == NULL)
    {
        g_free (contents);
        return FALSE;
    }

    if (fwrite ((const void *) contents, 1, length, backup_fd) != length)
        ret1 = FALSE;

    {
        int ret2;

        /* cppcheck-suppress redundantAssignment */
        ret2 = fflush (backup_fd);
        /* cppcheck-suppress redundantAssignment */
        ret2 = fclose (backup_fd);
        (void) ret2;
    }

    g_free (contents);
    return ret1;
}

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */

int
is_printable (int c)
{
    c &= 0xff;

#ifdef HAVE_CHARSET
    /* "Display bits" is ignored, since the user controls the output
       by setting the output codepage */
    return is_8bit_printable (c);
#else
    if (!mc_global.eight_bit_clean)
        return is_7bit_printable (c);

    if (mc_global.full_eight_bits)
        return is_8bit_printable (c);

    return is_iso_printable (c);
#endif /* !HAVE_CHARSET */
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Quote the filename for the purpose of inserting it into the command
 * line.  If quote_percent is TRUE, replace "%" with "%%" - the percent is
 * processed by the mc command line.
 */
char *
name_quote (const char *s, gboolean quote_percent)
{
    GString *ret;

    ret = g_string_sized_new (64);

    if (*s == '-')
        g_string_append (ret, "." PATH_SEP_STR);

    for (; *s != '\0'; s++)
    {
        switch (*s)
        {
        case '%':
            if (quote_percent)
                g_string_append_c (ret, '%');
            break;
        case '\'':
        case '\\':
        case '\r':
        case '\n':
        case '\t':
        case '"':
        case ';':
        case ' ':
        case '?':
        case '|':
        case '[':
        case ']':
        case '{':
        case '}':
        case '<':
        case '>':
        case '`':
        case '!':
        case '$':
        case '&':
        case '*':
        case '(':
        case ')':
            g_string_append_c (ret, '\\');
            break;
        case '~':
        case '#':
            if (ret->len == 0)
                g_string_append_c (ret, '\\');
            break;
        default:
            break;
        }
        g_string_append_c (ret, *s);
    }

    return g_string_free (ret, FALSE);
}

/* --------------------------------------------------------------------------------------------- */

char *
fake_name_quote (const char *s, gboolean quote_percent)
{
    (void) quote_percent;
    return g_strdup (s);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * path_trunc() is the same as str_trunc() but
 * it deletes possible password from path for security
 * reasons.
 */

const char *
path_trunc (const char *path, size_t trunc_len)
{
    vfs_path_t *vpath;
    char *secure_path;
    const char *ret;

    vpath = vfs_path_from_str (path);
    secure_path = vfs_path_to_str_flags (vpath, 0, VPF_STRIP_PASSWORD);
    vfs_path_free (vpath, TRUE);

    ret = str_trunc (secure_path, trunc_len);
    g_free (secure_path);

    return ret;
}

/* --------------------------------------------------------------------------------------------- */

const char *
size_trunc (uintmax_t size, gboolean use_si)
{
    static char x[BUF_TINY];
    uintmax_t divisor = 1;
    const char *xtra = _("B");

    if (size > 999999999UL)
    {
        divisor = use_si ? 1000 : 1024;
        xtra = use_si ? _("kB") : _("KiB");

        if (size / divisor > 999999999UL)
        {
            divisor = use_si ? (1000 * 1000) : (1024 * 1024);
            xtra = use_si ? _("MB") : _("MiB");

            if (size / divisor > 999999999UL)
            {
                divisor = use_si ? (1000 * 1000 * 1000) : (1024 * 1024 * 1024);
                xtra = use_si ? _("GB") : _("GiB");
            }
        }
    }
    g_snprintf (x, sizeof (x), "%.0f %s", 1.0 * size / divisor, xtra);
    return x;
}

/* --------------------------------------------------------------------------------------------- */

const char *
size_trunc_sep (uintmax_t size, gboolean use_si)
{
    static char x[60];
    int count;
    const char *p, *y;
    char *d;

    p = y = size_trunc (size, use_si);
    p += strlen (p) - 1;
    d = x + sizeof (x) - 1;
    *d-- = '\0';
    /* @size format is "size unit", i.e. "[digits][space][letters]".
       Copy all charactes after digits. */
    while (p >= y && !g_ascii_isdigit (*p))
        *d-- = *p--;
    for (count = 0; p >= y; count++)
    {
        if (count == 3)
        {
            *d-- = ',';
            count = 0;
        }
        *d-- = *p--;
    }
    d++;
    if (*d == ',')
        d++;
    return d;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Print file SIZE to BUFFER, but don't exceed LEN characters,
 * not including trailing 0. BUFFER should be at least LEN+1 long.
 * This function is called for every file on panels, so avoid
 * floating point by any means.
 *
 * Units: size units (filesystem sizes are 1K blocks)
 *    0=bytes, 1=Kbytes, 2=Mbytes, etc.
 */

void
size_trunc_len (char *buffer, unsigned int len, uintmax_t size, int units, gboolean use_si)
{
    /* Avoid taking power for every file.  */
    /* *INDENT-OFF* */
    static const uintmax_t power10[] = {
    /* we hope that size of uintmax_t is 4 bytes at least */
        1ULL,
        10ULL,
        100ULL,
        1000ULL,
        10000ULL,
        100000ULL,
        1000000ULL,
        10000000ULL,
        100000000ULL,
        1000000000ULL
    /* maximum value of uintmax_t (in case of 4 bytes) is
        4294967295
     */
#if SIZEOF_UINTMAX_T == 8
                     ,
        10000000000ULL,
        100000000000ULL,
        1000000000000ULL,
        10000000000000ULL,
        100000000000000ULL,
        1000000000000000ULL,
        10000000000000000ULL,
        100000000000000000ULL,
        1000000000000000000ULL,
        10000000000000000000ULL
    /* maximum value of uintmax_t (in case of 8 bytes) is
        18447644073710439615
     */
#endif
    };
    /* *INDENT-ON* */
    static const char *const suffix[] = { "", "K", "M", "G", "T", "P", "E", "Z", "Y", NULL };
    static const char *const suffix_lc[] = { "", "k", "m", "g", "t", "p", "e", "z", "y", NULL };

    const char *const *sfx = use_si ? suffix_lc : suffix;
    int j = 0;

    if (len == 0)
        len = 9;
#if SIZEOF_UINTMAX_T == 8
    /* 20 decimal digits are required to represent 8 bytes */
    else if (len > 19)
        len = 19;
#else
    /* 10 decimal digits are required to represent 4 bytes */
    else if (len > 9)
        len = 9;
#endif

    /*
     * recalculate from 1024 base to 1000 base if units>0
     * We can't just multiply by 1024 - that might cause overflow
     * if uintmax_t type is too small
     */
    if (use_si)
        for (j = 0; j < units; j++)
        {
            uintmax_t size_remain;

            size_remain = ((size % 125) * 1024) / 1000; /* size mod 125, recalculated */
            size /= 125;        /* 128/125 = 1024/1000 */
            size *= 128;        /* This will convert size from multiple of 1024 to multiple of 1000 */
            size += size_remain;        /* Re-add remainder lost by division/multiplication */
        }

    for (j = units; sfx[j] != NULL; j++)
    {
        if (size == 0)
        {
            if (j == units)
            {
                /* Empty files will print "0" even with minimal width.  */
                g_snprintf (buffer, len + 1, "%s", "0");
            }
            else
            {
                /* Use "~K" or just "K" if len is 1.  Use "B" for bytes.  */
                g_snprintf (buffer, len + 1, (len > 1) ? "~%s" : "%s", (j > 1) ? sfx[j - 1] : "B");
            }
            break;
        }

        if (size < power10[len - (j > 0 ? 1 : 0)])
        {
            g_snprintf (buffer, len + 1, "%" PRIuMAX "%s", size, sfx[j]);
            break;
        }

        /* Powers of 1000 or 1024, with rounding.  */
        if (use_si)
            size = (size + 500) / 1000;
        else
            size = (size + 512) >> 10;
    }
}

/* --------------------------------------------------------------------------------------------- */

const char *
string_perm (mode_t mode_bits)
{
    static char mode[11];

    strcpy (mode, "----------");
    if (S_ISDIR (mode_bits))
        mode[0] = 'd';
    if (S_ISCHR (mode_bits))
        mode[0] = 'c';
    if (S_ISBLK (mode_bits))
        mode[0] = 'b';
    if (S_ISLNK (mode_bits))
        mode[0] = 'l';
    if (S_ISFIFO (mode_bits))
        mode[0] = 'p';
    if (S_ISNAM (mode_bits))
        mode[0] = 'n';
    if (S_ISSOCK (mode_bits))
        mode[0] = 's';
    if (S_ISDOOR (mode_bits))
        mode[0] = 'D';
    if (ismode (mode_bits, S_IXOTH))
        mode[9] = 'x';
    if (ismode (mode_bits, S_IWOTH))
        mode[8] = 'w';
    if (ismode (mode_bits, S_IROTH))
        mode[7] = 'r';
    if (ismode (mode_bits, S_IXGRP))
        mode[6] = 'x';
    if (ismode (mode_bits, S_IWGRP))
        mode[5] = 'w';
    if (ismode (mode_bits, S_IRGRP))
        mode[4] = 'r';
    if (ismode (mode_bits, S_IXUSR))
        mode[3] = 'x';
    if (ismode (mode_bits, S_IWUSR))
        mode[2] = 'w';
    if (ismode (mode_bits, S_IRUSR))
        mode[1] = 'r';
#ifdef S_ISUID
    if (ismode (mode_bits, S_ISUID))
        mode[3] = (mode[3] == 'x') ? 's' : 'S';
#endif /* S_ISUID */
#ifdef S_ISGID
    if (ismode (mode_bits, S_ISGID))
        mode[6] = (mode[6] == 'x') ? 's' : 'S';
#endif /* S_ISGID */
#ifdef S_ISVTX
    if (ismode (mode_bits, S_ISVTX))
        mode[9] = (mode[9] == 'x') ? 't' : 'T';
#endif /* S_ISVTX */
    return mode;
}

/* --------------------------------------------------------------------------------------------- */

const char *
extension (const char *filename)
{
    const char *d;

    d = strrchr (filename, '.');

    return d != NULL ? d + 1 : "";
}

/* --------------------------------------------------------------------------------------------- */

char *
load_mc_home_file (const char *from, const char *filename, char **allocated_filename,
                   size_t * length)
{
    char *hintfile_base, *hintfile;
    char *lang;
    char *data;

    hintfile_base = g_build_filename (from, filename, (char *) NULL);
    lang = guess_message_value ();

    hintfile = g_strconcat (hintfile_base, ".", lang, (char *) NULL);
    if (!g_file_get_contents (hintfile, &data, length, NULL))
    {
        /* Fall back to the two-letter language code */
        if (lang[0] != '\0' && lang[1] != '\0')
            lang[2] = '\0';
        g_free (hintfile);
        hintfile = g_strconcat (hintfile_base, ".", lang, (char *) NULL);
        if (!g_file_get_contents (hintfile, &data, length, NULL))
        {
            g_free (hintfile);
            hintfile = hintfile_base;
            g_file_get_contents (hintfile_base, &data, length, NULL);
        }
    }

    g_free (lang);

    if (hintfile != hintfile_base)
        g_free (hintfile_base);

    if (allocated_filename != NULL)
        *allocated_filename = hintfile;
    else
        g_free (hintfile);

    return data;
}

/* --------------------------------------------------------------------------------------------- */

const char *
extract_line (const char *s, const char *top)
{
    static char tmp_line[BUF_MEDIUM];
    char *t = tmp_line;

    while (*s != '\0' && *s != '\n' && (size_t) (t - tmp_line) < sizeof (tmp_line) - 1 && s < top)
        *t++ = *s++;
    *t = '\0';
    return tmp_line;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * The basename routine
 */

const char *
x_basename (const char *s)
{
    const char *url_delim, *path_sep;

    url_delim = g_strrstr (s, VFS_PATH_URL_DELIMITER);
    path_sep = strrchr (s, PATH_SEP);

    if (path_sep == NULL)
        return s;

    if (url_delim == NULL
        || url_delim < path_sep - strlen (VFS_PATH_URL_DELIMITER)
        || url_delim - s + strlen (VFS_PATH_URL_DELIMITER) < strlen (s))
    {
        /* avoid trailing PATH_SEP, if present */
        if (!IS_PATH_SEP (s[strlen (s) - 1]))
            return path_sep + 1;

        while (--path_sep > s && !IS_PATH_SEP (*path_sep))
            ;
        return (path_sep != s) ? path_sep + 1 : s;
    }

    while (--url_delim > s && !IS_PATH_SEP (*url_delim))
        ;
    while (--url_delim > s && !IS_PATH_SEP (*url_delim))
        ;

    return url_delim == s ? s : url_delim + 1;
}

/* --------------------------------------------------------------------------------------------- */

const char *
unix_error_string (int error_num)
{
    static char buffer[BUF_LARGE];
    gchar *strerror_currentlocale;

    strerror_currentlocale = g_locale_from_utf8 (g_strerror (error_num), -1, NULL, NULL, NULL);
    g_snprintf (buffer, sizeof (buffer), "%s (%d)", strerror_currentlocale, error_num);
    g_free (strerror_currentlocale);

    return buffer;
}

/* --------------------------------------------------------------------------------------------- */

const char *
skip_separators (const char *s)
{
    const char *su = s;

    for (; *su != '\0'; str_cnext_char (&su))
        if (!whitespace (*su) && *su != ',')
            break;

    return su;
}

/* --------------------------------------------------------------------------------------------- */

const char *
skip_numbers (const char *s)
{
    const char *su = s;

    for (; *su != '\0'; str_cnext_char (&su))
        if (!str_isdigit (su))
            break;

    return su;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Remove all control sequences from the argument string.  We define
 * "control sequence", in a sort of pidgin BNF, as follows:
 *
 * control-seq = Esc non-'['
 *             | Esc '[' (0 or more digits or ';' or ':' or '?') (any other char)
 *
 * The 256-color and true-color escape sequences should allow either ';' or ':' inside as separator,
 * actually, ':' is the more correct according to ECMA-48.
 * Some terminal emulators (e.g. xterm, gnome-terminal) support this.
 *
 * Non-printable characters are also removed.
 */

char *
strip_ctrl_codes (char *s)
{
    char *w;                    /* Current position where the stripped data is written */
    char *r;                    /* Current position where the original data is read */

    if (s == NULL)
        return NULL;

    for (w = s, r = s; *r != '\0';)
    {
        if (*r == ESC_CHAR)
        {
            /* Skip the control sequence's arguments */ ;
            /* '(' need to avoid strange 'B' letter in *Suse (if mc runs under root user) */
            if (*(++r) == '[' || *r == '(')
            {
                /* strchr() matches trailing binary 0 */
                while (*(++r) != '\0' && strchr ("0123456789;:?", *r) != NULL)
                    ;
            }
            else if (*r == ']')
            {
                /*
                 * Skip xterm's OSC (Operating System Command)
                 * http://www.xfree86.org/current/ctlseqs.html
                 * OSC P s ; P t ST
                 * OSC P s ; P t BEL
                 */
                char *new_r;

                for (new_r = r; *new_r != '\0'; new_r++)
                {
                    switch (*new_r)
                    {
                        /* BEL */
                    case '\a':
                        r = new_r;
                        goto osc_out;
                    case ESC_CHAR:
                        /* ST */
                        if (new_r[1] == '\\')
                        {
                            r = new_r + 1;
                            goto osc_out;
                        }
                        break;
                    default:
                        break;
                    }
                }
              osc_out:
                ;
            }

            /*
             * Now we are at the last character of the sequence.
             * Skip it unless it's binary 0.
             */
            if (*r != '\0')
                r++;
        }
        else
        {
            char *n;

            n = str_get_next_char (r);
            if (str_isprint (r))
            {
                memmove (w, r, n - r);
                w += n - r;
            }
            r = n;
        }
    }

    *w = '\0';
    return s;
}

/* --------------------------------------------------------------------------------------------- */

enum compression_type
get_compression_type (int fd, const char *name)
{
    unsigned char magic[16];
    size_t str_len;

    /* Read the magic signature */
    if (mc_read (fd, (char *) magic, 4) != 4)
        return COMPRESSION_NONE;

    /* GZIP_MAGIC and OLD_GZIP_MAGIC */
    if (magic[0] == 037 && (magic[1] == 0213 || magic[1] == 0236))
        return COMPRESSION_GZIP;

    /* PKZIP_MAGIC */
    if (magic[0] == 0120 && magic[1] == 0113 && magic[2] == 003 && magic[3] == 004)
    {
        /* Read compression type */
        mc_lseek (fd, 8, SEEK_SET);
        if (mc_read (fd, (char *) magic, 2) != 2)
            return COMPRESSION_NONE;

        /* Gzip can handle only deflated (8) or stored (0) files */
        if ((magic[0] != 8 && magic[0] != 0) || magic[1] != 0)
            return COMPRESSION_NONE;

        /* Compatible with gzip */
        return COMPRESSION_GZIP;
    }

    /* PACK_MAGIC and LZH_MAGIC and compress magic */
    if (magic[0] == 037 && (magic[1] == 036 || magic[1] == 0240 || magic[1] == 0235))
        /* Compatible with gzip */
        return COMPRESSION_GZIP;

    /* BZIP and BZIP2 files */
    if ((magic[0] == 'B') && (magic[1] == 'Z') && (magic[3] >= '1') && (magic[3] <= '9'))
        switch (magic[2])
        {
        case '0':
            return COMPRESSION_BZIP;
        case 'h':
            return COMPRESSION_BZIP2;
        default:
            break;
        }

    /* LZ4 format - v1.5.0 - 0x184D2204 (little endian) */
    if (magic[0] == 0x04 && magic[1] == 0x22 && magic[2] == 0x4d && magic[3] == 0x18)
        return COMPRESSION_LZ4;

    if (mc_read (fd, (char *) magic + 4, 2) != 2)
        return COMPRESSION_NONE;

    /* LZIP files */
    if (magic[0] == 'L'
        && magic[1] == 'Z'
        && magic[2] == 'I' && magic[3] == 'P' && (magic[4] == 0x00 || magic[4] == 0x01))
        return COMPRESSION_LZIP;

    /* Support for LZMA (only utils format with magic in header).
     * This is the default format of LZMA utils 4.32.1 and later. */
    if (magic[0] == 0xFF
        && magic[1] == 'L'
        && magic[2] == 'Z' && magic[3] == 'M' && magic[4] == 'A' && magic[5] == 0x00)
        return COMPRESSION_LZMA;

    /* XZ compression magic */
    if (magic[0] == 0xFD
        && magic[1] == 0x37
        && magic[2] == 0x7A && magic[3] == 0x58 && magic[4] == 0x5A && magic[5] == 0x00)
        return COMPRESSION_XZ;

    if (magic[0] == 0x28 && magic[1] == 0xB5 && magic[2] == 0x2F && magic[3] == 0xFD)
        return COMPRESSION_ZSTD;

    str_len = strlen (name);
    /* HACK: we must belive to extension of LZMA file :) ... */
    if ((str_len > 5 && strcmp (&name[str_len - 5], ".lzma") == 0) ||
        (str_len > 4 && strcmp (&name[str_len - 4], ".tlz") == 0))
        return COMPRESSION_LZMA;

    return COMPRESSION_NONE;
}

/* --------------------------------------------------------------------------------------------- */

const char *
decompress_extension (int type)
{
    switch (type)
    {
    case COMPRESSION_GZIP:
        return "/ugz" VFS_PATH_URL_DELIMITER;
    case COMPRESSION_BZIP:
        return "/ubz" VFS_PATH_URL_DELIMITER;
    case COMPRESSION_BZIP2:
        return "/ubz2" VFS_PATH_URL_DELIMITER;
    case COMPRESSION_LZIP:
        return "/ulz" VFS_PATH_URL_DELIMITER;
    case COMPRESSION_LZ4:
        return "/ulz4" VFS_PATH_URL_DELIMITER;
    case COMPRESSION_LZMA:
        return "/ulzma" VFS_PATH_URL_DELIMITER;
    case COMPRESSION_XZ:
        return "/uxz" VFS_PATH_URL_DELIMITER;
    case COMPRESSION_ZSTD:
        return "/uzst" VFS_PATH_URL_DELIMITER;
    default:
        break;
    }
    /* Should never reach this place */
    fprintf (stderr, "Fatal: decompress_extension called with an unknown argument\n");
    return 0;
}

/* --------------------------------------------------------------------------------------------- */

void
wipe_password (char *passwd)
{
    if (passwd != NULL)
    {
        char *p;

        for (p = passwd; *p != '\0'; p++)
            *p = '\0';
        g_free (passwd);
    }
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Convert "\E" -> esc character and ^x to control-x key and ^^ to ^ key
 *
 * @param p pointer to string
 *
 * @return newly allocated string
 */

char *
convert_controls (const char *p)
{
    char *valcopy;
    char *q;

    valcopy = g_strdup (p);

    /* Parse the escape special character */
    for (q = valcopy; *p != '\0';)
        switch (*p)
        {
        case '\\':
            p++;

            if (*p == 'e' || *p == 'E')
            {
                p++;
                *q++ = ESC_CHAR;
            }
            break;

        case '^':
            p++;
            if (*p == '^')
                *q++ = *p++;
            else
            {
                char c;

                c = *p | 0x20;
                if (c >= 'a' && c <= 'z')
                {
                    *q++ = c - 'a' + 1;
                    p++;
                }
                else if (*p != '\0')
                    p++;
            }
            break;

        default:
            *q++ = *p++;
        }

    *q = '\0';
    return valcopy;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Finds out a relative path from first to second, i.e. goes as many ..
 * as needed up in first and then goes down using second
 */

char *
diff_two_paths (const vfs_path_t * vpath1, const vfs_path_t * vpath2)
{
    int j, prevlen = -1, currlen;
    char *my_first = NULL, *my_second = NULL;
    char *buf = NULL;

    my_first = resolve_symlinks (vpath1);
    if (my_first == NULL)
        goto ret;

    my_second = resolve_symlinks (vpath2);
    if (my_second == NULL)
        goto ret;

    for (j = 0; j < 2; j++)
    {
        char *p, *q;
        int i;

        p = my_first;
        q = my_second;

        while (TRUE)
        {
            char *r, *s;
            ptrdiff_t len;

            r = strchr (p, PATH_SEP);
            if (r == NULL)
                break;
            s = strchr (q, PATH_SEP);
            if (s == NULL)
                break;

            len = r - p;
            if (len != (s - q) || strncmp (p, q, (size_t) len) != 0)
                break;

            p = r + 1;
            q = s + 1;
        }
        p--;
        for (i = 0; (p = strchr (p + 1, PATH_SEP)) != NULL; i++)
            ;
        currlen = (i + 1) * 3 + strlen (q) + 1;
        if (j != 0)
        {
            if (currlen < prevlen)
                g_free (buf);
            else
                goto ret;
        }
        p = buf = g_malloc (currlen);
        prevlen = currlen;
        for (; i >= 0; i--, p += 3)
            strcpy (p, "../");
        strcpy (p, q);
    }

  ret:
    g_free (my_first);
    g_free (my_second);
    return buf;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Append text to GList, remove all entries with the same text
 */

GList *
list_append_unique (GList * list, char *text)
{
    GList *lc_link;

    /*
     * Go to the last position and traverse the list backwards
     * starting from the second last entry to make sure that we
     * are not removing the current link.
     */
    list = g_list_append (list, text);
    list = g_list_last (list);
    lc_link = g_list_previous (list);

    while (lc_link != NULL)
    {
        GList *newlink;

        newlink = g_list_previous (lc_link);
        if (strcmp ((char *) lc_link->data, text) == 0)
        {
            GList *tmp;

            g_free (lc_link->data);
            tmp = g_list_remove_link (list, lc_link);
            (void) tmp;
            g_list_free_1 (lc_link);
        }
        lc_link = newlink;
    }

    return list;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Read and restore position for the given filename.
 * If there is no stored data, return line 1 and col 0.
 */

void
load_file_position (const vfs_path_t * filename_vpath, long *line, long *column, off_t * offset,
                    GArray ** bookmarks)
{
    char *fn;
    FILE *f;
    char buf[MC_MAXPATHLEN + 100];
    const size_t len = vfs_path_len (filename_vpath);

    /* defaults */
    *line = 1;
    *column = 0;
    *offset = 0;

    /* open file with positions */
    fn = mc_config_get_full_path (MC_FILEPOS_FILE);
    f = fopen (fn, "r");
    g_free (fn);
    if (f == NULL)
        return;

    /* prepare array for serialized bookmarks */
    if (bookmarks != NULL)
        *bookmarks = g_array_sized_new (FALSE, FALSE, sizeof (size_t), MAX_SAVED_BOOKMARKS);

    while (fgets (buf, sizeof (buf), f) != NULL)
    {
        const char *p;
        gchar **pos_tokens;

        /* check if the filename matches the beginning of string */
        if (strncmp (buf, vfs_path_as_str (filename_vpath), len) != 0)
            continue;

        /* followed by single space */
        if (buf[len] != ' ')
            continue;

        /* and string without spaces */
        p = &buf[len + 1];
        if (strchr (p, ' ') != NULL)
            continue;

        pos_tokens = g_strsplit (p, ";", 3 + MAX_SAVED_BOOKMARKS);
        if (pos_tokens[0] == NULL)
        {
            *line = 1;
            *column = 0;
            *offset = 0;
        }
        else
        {
            *line = strtol (pos_tokens[0], NULL, 10);
            if (pos_tokens[1] == NULL)
            {
                *column = 0;
                *offset = 0;
            }
            else
            {
                *column = strtol (pos_tokens[1], NULL, 10);
                if (pos_tokens[2] == NULL)
                    *offset = 0;
                else if (bookmarks != NULL)
                {
                    size_t i;

                    *offset = (off_t) g_ascii_strtoll (pos_tokens[2], NULL, 10);

                    for (i = 0; i < MAX_SAVED_BOOKMARKS && pos_tokens[3 + i] != NULL; i++)
                    {
                        size_t val;

                        val = strtoul (pos_tokens[3 + i], NULL, 10);
                        g_array_append_val (*bookmarks, val);
                    }
                }
            }
        }

        g_strfreev (pos_tokens);
    }

    fclose (f);
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Save position for the given file
 */

void
save_file_position (const vfs_path_t * filename_vpath, long line, long column, off_t offset,
                    GArray * bookmarks)
{
    static size_t filepos_max_saved_entries = 0;
    char *fn, *tmp_fn;
    FILE *f, *tmp_f;
    char buf[MC_MAXPATHLEN + 100];
    size_t i;
    const size_t len = vfs_path_len (filename_vpath);
    gboolean src_error = FALSE;

    if (filepos_max_saved_entries == 0)
        filepos_max_saved_entries = mc_config_get_int (mc_global.main_config, CONFIG_APP_SECTION,
                                                       "filepos_max_saved_entries", 1024);

    fn = mc_config_get_full_path (MC_FILEPOS_FILE);
    if (fn == NULL)
        goto early_error;

    mc_util_make_backup_if_possible (fn, TMP_SUFFIX);

    /* open file */
    f = fopen (fn, "w");
    if (f == NULL)
        goto open_target_error;

    tmp_fn = g_strdup_printf ("%s" TMP_SUFFIX, fn);
    tmp_f = fopen (tmp_fn, "r");
    if (tmp_f == NULL)
    {
        src_error = TRUE;
        goto open_source_error;
    }

    /* put the new record */
    if (line != 1 || column != 0 || bookmarks != NULL)
    {
        if (fprintf
            (f, "%s %ld;%ld;%" PRIuMAX, vfs_path_as_str (filename_vpath), line, column,
             (uintmax_t) offset) < 0)
            goto write_position_error;
        if (bookmarks != NULL)
            for (i = 0; i < bookmarks->len && i < MAX_SAVED_BOOKMARKS; i++)
                if (fprintf (f, ";%zu", g_array_index (bookmarks, size_t, i)) < 0)
                    goto write_position_error;

        if (fprintf (f, "\n") < 0)
            goto write_position_error;
    }

    i = 1;
    while (fgets (buf, sizeof (buf), tmp_f) != NULL)
    {
        if (buf[len] == ' ' && strncmp (buf, vfs_path_as_str (filename_vpath), len) == 0
            && strchr (&buf[len + 1], ' ') == NULL)
            continue;

        fprintf (f, "%s", buf);
        if (++i > filepos_max_saved_entries)
            break;
    }

  write_position_error:
    fclose (tmp_f);
  open_source_error:
    g_free (tmp_fn);
    fclose (f);
    if (src_error)
        mc_util_restore_from_backup_if_possible (fn, TMP_SUFFIX);
    else
        mc_util_unlink_backup_if_possible (fn, TMP_SUFFIX);
  open_target_error:
    g_free (fn);
  early_error:
    if (bookmarks != NULL)
        g_array_free (bookmarks, TRUE);
}

/* --------------------------------------------------------------------------------------------- */

extern int
ascii_alpha_to_cntrl (int ch)
{
    if ((ch >= ASCII_A && ch <= ASCII_Z) || (ch >= ASCII_a && ch <= ASCII_z))
        ch &= 0x1f;

    return ch;
}

/* --------------------------------------------------------------------------------------------- */

const char *
Q_ (const char *s)
{
    const char *result, *sep;

    result = _(s);
    sep = strchr (result, '|');

    return sep != NULL ? sep + 1 : result;
}

/* --------------------------------------------------------------------------------------------- */

gboolean
mc_util_make_backup_if_possible (const char *file_name, const char *backup_suffix)
{
    struct stat stat_buf;
    char *backup_path;
    gboolean ret;

    if (!exist_file (file_name))
        return FALSE;

    backup_path = g_strdup_printf ("%s%s", file_name, backup_suffix);
    if (backup_path == NULL)
        return FALSE;

    ret = mc_util_write_backup_content (file_name, backup_path);
    if (ret)
    {
        /* Backup file will have same ownership with main file. */
        if (stat (file_name, &stat_buf) == 0)
            chmod (backup_path, stat_buf.st_mode);
        else
            chmod (backup_path, S_IRUSR | S_IWUSR);
    }

    g_free (backup_path);

    return ret;
}

/* --------------------------------------------------------------------------------------------- */

gboolean
mc_util_restore_from_backup_if_possible (const char *file_name, const char *backup_suffix)
{
    gboolean ret;
    char *backup_path;

    backup_path = g_strdup_printf ("%s%s", file_name, backup_suffix);
    if (backup_path == NULL)
        return FALSE;

    ret = mc_util_write_backup_content (backup_path, file_name);
    g_free (backup_path);

    return ret;
}

/* --------------------------------------------------------------------------------------------- */

gboolean
mc_util_unlink_backup_if_possible (const char *file_name, const char *backup_suffix)
{
    char *backup_path;

    backup_path = g_strdup_printf ("%s%s", file_name, backup_suffix);
    if (backup_path == NULL)
        return FALSE;

    if (exist_file (backup_path))
    {
        vfs_path_t *vpath;

        vpath = vfs_path_from_str (backup_path);
        mc_unlink (vpath);
        vfs_path_free (vpath, TRUE);
    }

    g_free (backup_path);
    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * partly taken from dcigettext.c, returns "" for default locale
 * value should be freed by calling function g_free()
 */

char *
guess_message_value (void)
{
    static const char *const var[] = {
        /* Setting of LC_ALL overwrites all other.  */
        /* Do not use LANGUAGE for check user locale and drowing hints */
        "LC_ALL",
        /* Next comes the name of the desired category.  */
        "LC_MESSAGES",
        /* Last possibility is the LANG environment variable.  */
        "LANG",
        /* NULL exit loops */
        NULL
    };

    size_t i;
    const char *locale = NULL;

    for (i = 0; var[i] != NULL; i++)
    {
        locale = getenv (var[i]);
        if (locale != NULL && locale[0] != '\0')
            break;
    }

    if (locale == NULL)
        locale = "";

    return g_strdup (locale);
}

/* --------------------------------------------------------------------------------------------- */

/**
 * The "profile root" is the tree under which all of MC's user data &
 * settings are stored.
 *
 * It defaults to the user's home dir. The user may override this default
 * with the environment variable $MC_PROFILE_ROOT.
 */
const char *
mc_get_profile_root (void)
{
    static const char *profile_root = NULL;

    if (profile_root == NULL)
    {
        profile_root = g_getenv ("MC_PROFILE_ROOT");
        if (profile_root == NULL || *profile_root == '\0')
            profile_root = mc_config_get_home_dir ();
    }

    return profile_root;
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Propagate error in simple way.
 *
 * @param dest error return location
 * @param code error code
 * @param format printf()-style format for error message
 * @param ... parameters for message format
 */

void
mc_propagate_error (GError ** dest, int code, const char *format, ...)
{
    if (dest != NULL && *dest == NULL)
    {
        GError *tmp_error;
        va_list args;

        va_start (args, format);
        tmp_error = g_error_new_valist (MC_ERROR, code, format, args);
        va_end (args);

        g_propagate_error (dest, tmp_error);
    }
}

/* --------------------------------------------------------------------------------------------- */
/**
 * Replace existing error in simple way.
 *
 * @param dest error return location
 * @param code error code
 * @param format printf()-style format for error message
 * @param ... parameters for message format
 */

void
mc_replace_error (GError ** dest, int code, const char *format, ...)
{
    if (dest != NULL)
    {
        GError *tmp_error;
        va_list args;

        va_start (args, format);
        tmp_error = g_error_new_valist (MC_ERROR, code, format, args);
        va_end (args);

        g_error_free (*dest);
        *dest = NULL;
        g_propagate_error (dest, tmp_error);
    }
}

/* --------------------------------------------------------------------------------------------- */

/**
 * Returns if the given duration has elapsed since the given timestamp,
 * and if it has then updates the timestamp.
 *
 * @param timestamp the last timestamp in microseconds, updated if the given time elapsed
 * @param delay amount of time in microseconds

 * @return TRUE if clock skew detected, FALSE otherwise
 */
gboolean
mc_time_elapsed (gint64 * timestamp, gint64 delay)
{
    gint64 now;

    now = g_get_real_time ();

    if (now >= *timestamp && now < *timestamp + delay)
        return FALSE;

    *timestamp = now;
    return TRUE;
}

/* --------------------------------------------------------------------------------------------- */
