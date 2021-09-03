/**
 * @brief The description of Builtin dynamic variants.
 * \defgroup builtin_vars Builtin Variables
 * @{
 */

/**
 * \defgroup bv_system $SYSTEM
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
 *      $SYSTEM.uname
 * @endcode
 *
 * @sa          uname_prt
 */
object uname;


/**
 * @var string locale
 * @brief Get locale information for LC_MESSAGES.
 *
 * @return A string which contains LC_MESSAGES locale information.
 * @par sample
 * @code
 *      $SYSTEM.locale
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
 *      $SYSTEM.random
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
 *      $SYSTEM.time
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
 *              $SYSTEM.uname_prt('kernel-name kernel-release kernel-version')
 *
 *              // get all system informations 
 *              $SYSTEM.uname_prt('all')
 * @endcode
 * @note        If input "all", you will get all system information as $SYSTEM.uname.\n
 *              If input "default", only get "kernel-name", "kernel-release", "kernel-version", "nodename", "machine" information.
 *
 * @sa          uname
 */
string uname_prt(string keywords_list);


/**
 * @brief       Get or Set locale information according to user's input 
 *
 * @param[in]   category : which category information you will get or set.\n 
 *              GETTER: ctype | numeric | time | collate | monetary | messages | paper | name | address | telephone | measurement | identification
 *              SETTER: [ctype || numeric || time || collate || monetary || messages || paper || name || address || telephone || measurement || identification] | all
 * @param[in]   locale : which locale you will set.\n 
 *              GETTER: None
 *              SETTER: <string: locale> 
 *
 * @return      GETTER : A string contains the indicated locale information.
 *              SETTER : A boolean variable, true for successful, otherwise false.
 *
 * @par sample
 * @code
 *              // get locale for LC_COLLATE
 *              $SYSTEM.locale('collate')
 *
 *              // set local for LC_COLLATE
 *              $SYSTEM.locale(!'collate', "en_US.UTF-8")
 * @endcode
 */
string locale (string category, string locale);



/**
 * @brief       Get a random, the range is from 0 to user defined. 
 *
 * @param[in]   max_range : maxium random
 *
 * @return      A random number whose is from 0 to max_range.
 *
 * @par sample
 * @code
 *              $SYSTEM.random (3.1415926)
 * @endcode
 */
number random (number max_range);


/**
 * @brief       Get or Set time. 
 *
 * @param[in]   format    : A varaint array
 *                        argv[0], a PURC_VARIANT_TYPE_STRING type, time format
 *                            -"tm": get the time information as struct tm;
 *                            -"iso8601": get the time in iso8601 format;
 *                            -"RFC822": get the time in RFC822 format;
 *                            -format string: get the time with string user defined
 *                        argv[1], seconds since the Epoch, can be 
 *                            PURC_VARIANT_TYPE_NUMBER, 
 *                            PURC_VARIANT_TYPE_ULONGINT,
 *                            PURC_VARIANT_TYPE_LONGDOUBLE, 
 *                            PURC_VARIANT_TYPE_LONGINT type
 *                        argv[2], a PURC_VARIANT_TYPE_STRING type, timezone
 *
 * @return
 *              - PURC_VARIANT_INVALID, with errno:
 *                  PURC_ERROR_INVALID_VALUE when creates variant error,
 *                  PURC_ERROR_WRONG_ARGS when input parameter is error.
 *              - A PURC_VARIANT_TYPE_STRING varint for the time information.
 *
 * @par sample
 * @code
 *              purc_variant_t sys = pcdvojbs_get_system();
 *              purc_variant_t dynamic = purc_variant_object_get_c (sys, "time");
 *              purc_dvariant_method func = 
 *                                  purc_variant_dynamic_get_getter (dynamic);
 *
 *              // get the current time in iso8601 format
 *              purc_variant_t param[4];
 *              param[0] = purc_variant_make_string ("iso8601");
 *              param[1] = PURC_VARIANT_UNDEFINED;
 *              param[2] = PURC_VARIANT_UNDEFINED;
 *              param[3] = NULL;
 *              purc_variant_t var1 = func (root, 3, param);
 *
 *              // get time in Asia/Shanghai, and Epoch is 1234567,
 *              // and return string format is iso8601
 *              param[0] = purc_variant_make_string ("iso8601");
 *              param[1] = purc_variant_make_number (1234567);
 *              param[2] = purc_variant_make_string ("Asia/Shanghai");
 *              param[3] = NULL;
 *              purc_variant_t var2 = func (root, 3, param);
 *
 *              // get time in Asia/Shanghai, and Epoch is 1234567,
 *              // and return string is in user defined format
 *              param[0] = purc_variant_make_string ("The Shanghai time is %H:%m");
 *              param[1] = purc_variant_make_number (1234567);
 *              param[2] = purc_variant_make_string ("Asia/Shanghai");
 *              param[3] = NULL;
 *              purc_variant_t var3 = func (root, 3, param);
 * @endcode
 *
 * @note        If do not indicate seconds since the Epoch, or time zone, 
 *              argv[1] and argv[2] should be PURC_VARIANT_UNDEFINED.
 *              Support user define format, as below:
 *                  %Y: the year
 *                  %m: the month
 *                  %d: the day
 *                  %H: the hour
 *                  %M: the minute
 *                  %S: the second
 */
string time (string format, number epoch, string locale);


/**
 * @brief       Set time. 
 *
 * @param[in]   root    : The context variant
 * @param[in]   nr_args : The number of elements in argv array, it should be 1
 * @param[in]   argv    : A varaint array
 *                        argv[0], seconds since the Epoch, it should be 
 *                            PURC_VARIANT_TYPE_NUMBER type.
 *
 * @return
 *              - PURC_VARIANT_INVALID, with errno:
 *                  PURC_ERROR_INVALID_VALUE when creates variant error,
 *                  PURC_ERROR_WRONG_ARGS when input parameter is error.
 *              - A PURC_VARIANT_TYPE_BOOLEAN varint.
 *                  PURC_VARIANT_TRUE for successful, otherwise 
 *                  PURC_VARIANT_FALSE.
 *
 * @par sample
 * @code
 *              purc_variant_t sys = pcdvojbs_get_system();
 *              purc_variant_t dynamic = purc_variant_object_get_c (sys, "time");
 *              purc_dvariant_method func = 
 *                                  purc_variant_dynamic_get_setter (dynamic);
 *
 *              // get the current time in iso8601 format
 *              purc_variant_t param[2];
 *              param[0] = purc_variant_make_number (1234567);
 *              param[1] = NULL;
 *              purc_variant_t var = func (root, 1, param);
 * @endcode
 *
 * @note 
 */
purc_variant_t
time_setter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
}

/** @} end of bv_system */
/** @} end of builtin_vars */

