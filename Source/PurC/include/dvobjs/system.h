/**
 * @brief The description of Builtin dynamic variants.
 * \defgroup builtin_vars Builtin Variables
 * @{
 */

/**
 * \defgroup bv_system $SYS
 * @{
 */

/**
 * @var object uname
 * @brief Get all system information.
 *
 * @return An object which contains all system information.
 * @code
 *      {
 *          "kernel-name"       : "xxxxxxx",
 *          "nodename"          : "xxxxxxx",
 *          "kernel-release"    : "xxxxxxx",
 *          "kernel-version"    : "xxxxxxx",
 *          "machine"           : "xxxxxxx",
 *          "processor"         : "xxxxxxx",
 *          "hardware-platform" : "xxxxxxx",
 *          "operating-system"  : "xxxxxxx"
 *      }
 * @endcode
 * @par sample
 * @code
 *      $SYS.uname
 * @endcode
 *
 * @sa          uname_prt()
 */
object uname;


/**
 * @var string locale
 * @brief Get locale information for LC_MESSAGES.
 *
 * @return A string which contains LC_MESSAGES locale information.
 * @par sample
 * @code
 *      $SYS.locale
 * @endcode
 *
 */
string locale;

/**
 * @var number random 
 * @brief Get a random number, whose range is from 0 to MRAND_MAX.
 *
 * @return A random number.
 * @par sample
 * @code
 *      $SYS.random
 * @endcode
 *
 */
number random;


/**
 * @var number time
 * @brief Get seconds from Epoch.
 *
 * @return A number which is the seconds from Epoch.
 * @par sample
 * @code
 *      $SYS.time
 * @endcode
 *
 */
number time;


/**
 * @brief       Get system information according to user's input 
 *
 * @param[in]   keywords_list : which system information you will get.\n
 *              [kernel-name || kernel-release || kernel-version || nodename || machine || processor || hardware-platform || operating-system] | all | default
 *
 * @return      A string contains the system information items, accroding to the input orders with a space as delimiter.
 *
 * @par sample
 * @code
 *              // get only kernel-name kernel-release kernel-version
 *              $SYS.uname_prt('kernel-name kernel-release kernel-version')
 *
 *              // get all system informations 
 *              $SYS.uname_prt('all')
 * @endcode
 * @note        If input "all", you will get all system information as $SYS.uname.\n
 *              If input "default", only get "kernel-name", "kernel-release", "kernel-version", "nodename", "machine" information.
 *
 * @sa          uname
 */
string uname_prt(string keywords_list);


/**
 * @brief       Get locale information according to user's input 
 *
 * @param[in]   category : which category information you will get or set.\n 
 *              ctype | numeric | time | collate | monetary | messages | paper | name | address | telephone | measurement | identification
 *
 * @return      A string contains the indicated locale information.
 *
 * @par sample
 * @code
 *              // get locale for LC_COLLATE
 *              $SYS.locale('collate')
 * @endcode
 */
string locale(string category)  GETTER;


/**
 * @brief       Set locale information according to user's input 
 *
 * @param[in]   category : which category information you will get or set.\n 
 *              [ctype || numeric || time || collate || monetary || messages || paper || name || address || telephone || measurement || identification] | all
 * @param[in]   locale : which locale you will set.\n 
 *
 * @return      A boolean variant, true for successful, otherwise false.
 *
 * @par sample
 * @code
 *              $SYS.locale(! 'collate', "en_US.UTF-8")
 * @endcode
 */
boolean locale(string category, string locale)  SETTER;



/**
 * @brief       Get a random, the range is from 0 to user defined. 
 *
 * @param[in]   max_range : maxium random
 *
 * @return      A random number whose is from 0 to max_range.
 *
 * @par sample
 * @code
 *              $SYS.random(3.1415926)
 * @endcode
 */
number random(number max_range);


/**
 * @brief       Get time information in string format. 
 *
 * @param[in]   format    : time string format\n
 *                          -"tm": get the time information as struct tm;\n
 *                          -"ISO8601": get the time in ISO8601 format;\n
 *                          -"RFC822": get the time in RFC822 format;\n
 *                          -format string: get the time with string user defined
 * @param[in]   epoch     : seconds since the Epoch
 * @param[in]   locale    : timezone
 *
 * @return      A string with time information.
 *
 * @par sample
 * @code
 *              // get time with ISO8601 format
 *              $SYS.time("ISO8601");
 *
 *              // get time in Asia/Shanghai, and Epoch is 1234567,
 *              $SYS.time("ISO8601", 1234567, "Asia/Shanghai");
 *
 *              // get time in Asia/Shanghai, and Epoch is 1234567,
 *              // and return string is in user defined format
 *              $SYS.time("The Shanghai time is %H:%m", 1234567, "Asia/Shanghai");
 * @endcode
 *
 * @note
 * @code
 *              multiple_type can be:
 *                  number
 *                  longint
 *                  ulongint
 *                  longdouble
 * @endcode
 *
 * @par
 * @code
 *              Supported user define format, as below:
 *                  %%Y: the year
 *                  %%m: the month
 *                  %%d: the day
 *                  %%H: the hour
 *                  %%M: the minute
 *                  %%S: the second
 * @endcode
 */
string time(string format [, multiple_type epoch[, string: timezone]])  GETTER;


/**
 * @brief       Set time. 
 *
 * @param[in]   epoch     : seconds since the Epoch
 *
 * @return      A boolean, true for successfully, otherwise false.
 *
 * @par sample
 * @code
 *              // set time, and Epoch is 1234567
 *              $SYS.time(! 1234567);
 * @endcode
 */
boolean time(number epoch)  SETTER;

/** @} end of bv_system */
/** @} end of builtin_vars */
