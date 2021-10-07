/**
 * @brief The description of Builtin dynamic variants.
 * \defgroup builtin_vars Builtin Variables
 * @{
 */

/**
 * \defgroup bv_string $STR
 * @{
 */

/**
 * @brief       Whether string haystack contains substring string needle
 *
 * @param[in]   haystack : the source string
 * @param[in]   needle   : the substring to be found
 *
 * @return      A boolean variant, true for successful, otherwise false.
 *
 * @par sample
 * @code
 *              $STR.contains("hello world", "china")
 *
 * @endcode
 * @note
 *
 * @sa          ends_with()  explode()  shuffle()  replace()
 */
boolean contains(string haystack, string needle);


/**
 * @brief       Whether string haystack ends with substring string needle
 *
 * @param[in]   haystack : the source string
 * @param[in]   needle   : the substring
 *
 * @return      A boolean variant, true for successful, otherwise false.
 *
 * @par sample
 * @code
 *              $STR.ends_with("hello world", "hello")
 *
 * @endcode
 * @note
 *
 * @sa          contains()  explode()  shuffle()  replace()
 */
boolean ends_with(string haystack, string needle);


/**
 * @brief       Seperate the source string with delimit string
 *
 * @param[in]   string    : the source string
 * @param[in]   delimits  : the string to delimit source string
 *
 * @return      A array variant, each element is the substring seperated by delimit string.
 *
 * @par sample
 * @code
 *              $STR.explode("hello world", "he")
 *
 * @endcode
 * @note
 *
 * @sa          contains()  ends_with()  shuffle()  replace()
 */
array explode(string string, string delimits);


/**
 * @brief       Rebuild a new string, every letter is in random order. 
 *
 * @param[in]   string    : the source string
 *
 * @return      A string variant, with random order.
 *
 * @par sample
 * @code
 *              $STR.shuffle("hello world")
 *
 * @endcode
 * @note
 *
 * @sa          contains()  ends_with()  explode()  replace()
 */
string shuffle(string string);


/**
 * @brief       Replace string oldstr with string newstr, in source string. 
 *
 * @param[in]   string    : the source string
 * @param[in]   oldstr    : the substring to be replaced
 * @param[in]   newstr    : the replaced string
 *
 * @return      A string variant, with replaced substring.
 *
 * @par sample
 * @code
 *              $STR.replace("hello world", "world", "china")
 *
 * @endcode
 * @note
 *
 * @sa          contains()  ends_with()  explode()  shuffle()
 */
string replace(string string, string oldstr, string newstr);


/**
 * @brief       FORMAT a string as in C printf. 
 *
 * @param[in]   format    : the source string with format control
 * @param[in]   value     : the value to replace the control in format string
 *
 * @return      A string variant, with format string
 *
 * @par sample
 * @code
 *              $STR.format_c("hello %s", "world")
 *
 * @endcode
 * @note
 * @code
 *              multiple_type can be:
 *                  boolean 
 *                  number
 *                  longint
 *                  ulongint
 *                  longdouble
 *                  string
 * @endcode
 *
 * @par
 * @code
 *              Supported user define format, as below:
 *                  %d: the integer
 *                  %o: the number in octal
 *                  %u: the unsinged integer
 *                  %x: the number in hexadecimal
 *                  %f: the double
 *                  %s: the string
 * @endcode
 *
 * @sa          format_p()
 */
string format_c(string format [, multiple_type value [, ...]]);


/**
 * @brief       Replace placeholder with element in array or object. 
 *
 * @param[in]   string    : the source string with placeholder
 * @param[in]   elements  : the array or object variant, with the elements to replace the placeholder in string
 *
 * @return      A string variant
 *
 * @par sample
 * @code
 *              $STR.format_p('There are two boys: {0} and {1}', ['Tom', 'Jerry'])
 *              $STR.format_p('There are two boys: {name0} and {name1}', { name0: 'Tom', name1: 'Jerry'})
 *
 * @endcode
 * @note
 * @code
 *              multiple_type can be:
 *                  array 
 *                  object
 * @endcode
 * @sa          format_c()
 */
string format_p(<string: string>, [multiple_type elements]);



/** @} end of bv_string */
/** @} end of builtin_vars */
