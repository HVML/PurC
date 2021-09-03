/**
 * @brief The description of Builtin LOGICAL dynamic variants.
 * \defgroup builtin_vars Builtin Variables
 * @{
 */

/**
 * \defgroup bv_logical $L
 * @{
 */

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
boolean not(string keywords_list)
{
}


/** @} end of bv_logical */
/** @} end of builtin_vars */

