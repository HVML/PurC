/* libwapcaplet.h
 *
 * String internment and management tools.
 *
 * Copyright 2009 The NetSurf Browser Project.
 *		  Daniel Silverstone <dsilvers@netsurf-browser.org>
 */

#ifndef libwapcaplet_h_
#define libwapcaplet_h_

#include <sys/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of a reference counter used in libwapcaplet.
 */
typedef uint32_t lwc_refcounter;
	
/**
 * The type of a hash value used in libwapcaplet.
 */
typedef uint32_t lwc_hash;

/**
 * An interned string.
 *
 * NOTE: The contents of this struct are considered *PRIVATE* and may
 * change in future revisions.  Do not rely on them whatsoever.
 * They're only here at all so that the ref, unref and matches etc can
 * use them.
 */
typedef struct lwc_string_s {
        struct lwc_string_s **	prevptr;
        struct lwc_string_s *	next;
        size_t		len;
        lwc_hash	hash;
        lwc_refcounter	refcnt;
        struct lwc_string_s *	insensitive;
} lwc_string;
	
/**
 * String iteration function
 *
 * @param str A string which has been interned.
 * @param pw The private pointer for the allocator.
 */
typedef void (*lwc_iteration_callback_fn)(lwc_string *str, void *pw);

/**
 * Result codes which libwapcaplet might return.
 */
typedef enum lwc_error_e {
	lwc_error_ok		= 0,	/**< No error. */
	lwc_error_oom		= 1,	/**< Out of memory. */
	lwc_error_range		= 2	/**< Substring internment out of range. */
} lwc_error;

/**
 * Intern a string.
 *
 * Take a copy of the string data referred to by \a s and \a slen and
 * intern it.  The resulting ::lwc_string can be used for simple and
 * caseless comparisons by ::lwc_string_isequal and
 * ::lwc_string_caseless_isequal respectively.
 *
 * @param s    Pointer to the start of the string to intern.
 * @param slen Length of the string in characters. (Not including any 
 *	       terminators)
 * @param ret  Pointer to ::lwc_string pointer to fill out.
 * @return     Result of operation, if not OK then the value pointed
 *	       to by \a ret will not be valid.
 *
 * @note The memory pointed to by \a s is not referenced by the result.
 * @note If the string was already present, its reference count is
 * incremented rather than allocating more memory.
 *
 * @note The returned string is currently NULL-terminated but this
 *	 will not necessarily be the case in future.  Try not to rely
 *	 on it.
 */
extern lwc_error lwc_intern_string(const char *s, size_t slen,
                                   lwc_string **ret);

/**
 * Intern a substring.
 *
 * Intern a subsequence of the provided ::lwc_string.
 *
 * @param str	   String to acquire substring from.
 * @param ssoffset Substring offset into \a str.
 * @param sslen	   Substring length.
 * @param ret	   Pointer to pointer to ::lwc_string to fill out.
 * @return	   Result of operation, if not OK then the value
 *		   pointed to by \a ret will not be valid.
 */
extern lwc_error lwc_intern_substring(lwc_string *str,
                                      size_t ssoffset, size_t sslen,
                                      lwc_string **ret);

/**
 * Optain a lowercased lwc_string from given lwc_string.
 *
 * @param str  String to create lowercase string from.
 * @param ret  Pointer to ::lwc_string pointer to fill out.
 * @return     Result of operation, if not OK then the value pointed
 *             to by \a ret will not be valid.
 */
extern lwc_error lwc_string_tolower(lwc_string *str, lwc_string **ret);

/**
 * Increment the reference count on an lwc_string.
 *
 * This increases the reference count on the given string. You should
 * use this when copying a string pointer into a persistent data
 * structure.
 *
 * @verbatim
 *   myobject->str = lwc_string_ref(myparent->str);
 * @endverbatim
 *
 * @param str The string to create another reference to.
 * @return    The string pointer to use in your new data structure.
 *
 * @note Use this if copying the string and intending both sides to retain
 * ownership.
 */
#if defined(STMTEXPR)
#define lwc_string_ref(str) ({lwc_string *__lwc_s = (str); assert(__lwc_s != NULL); __lwc_s->refcnt++; __lwc_s;})
#else
static inline lwc_string *
lwc_string_ref(lwc_string *str)
{
	assert(str != NULL);
	str->refcnt++;
	return str;
}
#endif

/**
 * Release a reference on an lwc_string.
 *
 * This decreases the reference count on the given ::lwc_string.
 *
 * @param str The string to unref.
 *
 * @note If the reference count reaches zero then the string will be
 *       freed. (Ref count of 1 where string is its own insensitve match
 *       will also result in the string being freed.)
 */
#define lwc_string_unref(str) {						\
		lwc_string *__lwc_s = (str);				\
		assert(__lwc_s != NULL);				\
		__lwc_s->refcnt--;						\
		if ((__lwc_s->refcnt == 0) ||					\
		    ((__lwc_s->refcnt == 1) && (__lwc_s->insensitive == __lwc_s)))	\
			lwc_string_destroy(__lwc_s);				\
	}
	
/**
 * Destroy an unreffed lwc_string.
 *
 * This destroys an lwc_string whose reference count indicates that it should be.
 *
 * @param str The string to unref.
 */
extern void lwc_string_destroy(lwc_string *str);

/**
 * Check if two interned strings are equal.
 *
 * @param str1 The first string in the comparison.
 * @param str2 The second string in the comparison.
 * @param ret  A pointer to a boolean to be filled out with the result.
 * @return     Result of operation, if not ok then value pointed to
 *	       by \a ret will not be valid.
 */
#define lwc_string_isequal(str1, str2, ret) \
	((*(ret) = ((str1) == (str2))), lwc_error_ok)

/**
 * Intern a caseless copy of the passed string.
 *
 * @param str The string to intern the caseless copy of.
 *
 * @return    lwc_error_ok if successful, otherwise the
 *            error code describing the issue.,
 *
 * @note This is for "internal" use by the caseless comparison
 *       macro and not for users.
 */
extern lwc_error
lwc__intern_caseless_string(lwc_string *str);

#if defined(STMTEXPR)
/**
 * Check if two interned strings are case-insensitively equal.
 *
 * @param _str1 The first string in the comparison.
 * @param _str2 The second string in the comparison.
 * @param _ret  A pointer to a boolean to be filled out with the result.
 * @return Result of operation, if not ok then value pointed to by \a ret will
 *	    not be valid.
 */
#define lwc_string_caseless_isequal(_str1,_str2,_ret) ({                \
            lwc_error __lwc_err = lwc_error_ok;                         \
            lwc_string *__lwc_str1 = (_str1);                           \
            lwc_string *__lwc_str2 = (_str2);                           \
            bool *__lwc_ret = (_ret);                                   \
                                                                        \
            if (__lwc_str1->insensitive == NULL) {                      \
                __lwc_err = lwc__intern_caseless_string(__lwc_str1);    \
            }                                                           \
            if (__lwc_err == lwc_error_ok && __lwc_str2->insensitive == NULL) { \
                __lwc_err = lwc__intern_caseless_string(__lwc_str2);    \
            }                                                           \
            if (__lwc_err == lwc_error_ok)                              \
                *__lwc_ret = (__lwc_str1->insensitive == __lwc_str2->insensitive); \
            __lwc_err;                                                  \
        })
	
#else
/**
 * Check if two interned strings are case-insensitively equal.
 *
 * @param str1 The first string in the comparison.
 * @param str2 The second string in the comparison.
 * @param ret  A pointer to a boolean to be filled out with the result.
 * @return Result of operation, if not ok then value pointed to by \a ret will
 *         not be valid.
 */
static inline lwc_error
lwc_string_caseless_isequal(lwc_string *str1, lwc_string *str2, bool *ret)
{
       lwc_error err = lwc_error_ok;
       if (str1->insensitive == NULL) {
           err = lwc__intern_caseless_string(str1);
       }
       if (err == lwc_error_ok && str2->insensitive == NULL) {
           err = lwc__intern_caseless_string(str2);
       }
       if (err == lwc_error_ok)
           *ret = (str1->insensitive == str2->insensitive);
       return err;
}
#endif

#if defined(STMTEXPR)
#define lwc__assert_and_expr(str, expr) ({assert(str != NULL); expr;})
#else
#define lwc__assert_and_expr(str, expr) (expr)
#endif
	
/**
 * Retrieve the data pointer for an interned string.
 *
 * @param str The string to retrieve the data pointer for.
 * @return    The C string data pointer for \a str.
 *
 * @note The data we point at belongs to the string and will
 *	 die with the string. Keep a ref if you need it.
 * @note You may not rely on the NULL termination of the strings
 *	 in future.  Any code relying on it currently should be
 *	 modified to use ::lwc_string_length if possible.
 */
#define lwc_string_data(str) lwc__assert_and_expr(str, (const char *)((str)+1))

/**
 * Retrieve the data length for an interned string.
 *
 * @param str The string to retrieve the length of.
 * @return    The length of \a str.
 */
#define lwc_string_length(str) lwc__assert_and_expr(str, (str)->len)

/**
 * Retrieve (or compute if unavailable) a hash value for the content of the string.
 *
 * @param str The string to get the hash for.
 * @return    The 32 bit hash of \a str.
 *
 * @note This API should only be used as a convenient way to retrieve a hash
 *	 value for the string. This hash value should not be relied on to be
 *	 unique within an invocation of the program, nor should it be relied upon
 *	 to be stable between invocations of the program. Never use the hash
 *	 value as a way to directly identify the value of the string.
 */
#define lwc_string_hash_value(str) lwc__assert_and_expr(str, (str)->hash)

/**
 * Retrieve a hash value for the caseless content of the string.
 *
 * @param str   The string to get caseless hash value for.
 * @param hash  A pointer to a hash value to be filled out with the result.
 * @return Result of operation, if not ok then value pointed to by \a ret will
 *      not be valid.
 */
static inline lwc_error lwc_string_caseless_hash_value(
	lwc_string *str, lwc_hash *hash)
{
	if (str->insensitive == NULL) {
		lwc_error err = lwc__intern_caseless_string(str);
		if (err != lwc_error_ok) {
			return err;
		}
	}

	*hash = str->insensitive->hash;
	return lwc_error_ok;
}


/**
 * Iterate the context and return every string in it.
 *
 * If there are no strings found in the context, then this has the
 * side effect of removing the global context which will reduce the
 * chances of false-positives on leak checkers.
 *
 * @param cb The callback to give the string to.
 * @param pw The private word for the callback.
 */
extern void lwc_iterate_strings(lwc_iteration_callback_fn cb, void *pw);

#ifdef __cplusplus
}
#endif

#endif /* libwapcaplet_h_ */
