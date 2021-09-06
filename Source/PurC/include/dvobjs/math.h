/**
 * @brief The description of Loadable dynamic variants.
 * \defgroup loadable_vars Loadable Variables
 * @{
 */

/**
 * \defgroup lv_math $MATH
 * @{
 */

/**
 * @var double pi
 * @brief Get pi with double type.
 *
 * @return A double number of pi value.
 *
 * @par sample
 * @code
 *      $MATH.pi
 * @endcode
 *
 * @sa          pi_l
 */
double pi;


/**
 * @var long double pi
 * @brief Get pi with long double type.
 *
 * @return A long double number of pi value.
 *
 * @par sample
 * @code
 *      $MATH.pi_l
 * @endcode
 *
 * @sa          pi
 */
long double pi_l;


/**
 * @var double e
 * @brief Get e with double type.
 *
 * @return A double number of e value.
 *
 * @par sample
 * @code
 *      $MATH.e
 * @endcode
 *
 * @sa          e_l
 */
double e;


/**
 * @var long double e
 * @brief Get e with long double type.
 *
 * @return A long double number of e value.
 *
 * @par sample
 * @code
 *      $MATH.e_l
 * @endcode
 *
 * @sa          e
 */
long double e_l;


/**
 * @brief       Get usually const with double value 
 *
 * @param[in]   keywords : which const you will get.\n
 *              e | log2e | log10e | ln2 | ln10 | pi | pi/2 | pi/4 | 1/pi | 2/pi | sqrt(2) | 2/sqrt(pi) | 1/sqrt(2)
 *
 * @return      A double number accroding to the input string.
 *
 * @par sample
 * @code
 *              $MATH.const('2/pi')
 * @endcode
 *
 * @note        
 *
 * @sa          const_l()
 */
double const(string);


/**
 * @brief       Get usually const with long double value 
 *
 * @param[in]   keywords : which const you will get.\n
 *              e | log2e | log10e | ln2 | ln10 | pi | pi/2 | pi/4 | 1/pi | 2/pi | sqrt(2) | 2/sqrt(pi) | 1/sqrt(2)
 *
 * @return      A long double number accroding to the input string.
 *
 * @par sample
 * @code
 *              $MATH.const_l('2/pi')
 * @endcode
 *
 * @note        
 *
 * @sa          const()
 */
long double const_l(string);


/**
 * @brief       Get sin value with double type 
 *
 * @param[in]   angle : the angle given in radians
 *
 * @return      sin value with double type.
 *
 * @par sample
 * @code
 *              $MATH.sin(1.0)
 * @endcode
 *
 * @note        The angle is given in RADIANS.
 *
 * @sa          sin_l()
 */
double sin(double angle);


/**
 * @brief       Get sin value with long double type 
 *
 * @param[in]   angle : the angle given in radians
 *
 * @return      sin value with long double type.
 *
 * @par sample
 * @code
 *              $MATH.sin_l(1.0)
 * @endcode
 *
 * @note        The angle is given in RADIANS.
 *
 * @sa          sin()
 */
long double sin_l(long double angle);


/**
 * @brief       Get cos value with double type 
 *
 * @param[in]   angle : the angle given in radians
 *
 * @return      cos value with double type.
 *
 * @par sample
 * @code
 *              $MATH.cos(1.0)
 * @endcode
 *
 * @note        The angle is given in RADIANS.
 *
 * @sa          cos_l()
 */
double cos(double angle);


/**
 * @brief       Get cos value with long double type 
 *
 * @param[in]   angle : the angle given in radians
 *
 * @return      sin value with long double type.
 *
 * @par sample
 * @code
 *              $MATH.cos_l(1.0)
 * @endcode
 *
 * @note        The angle is given in RADIANS.
 *
 * @sa          cos()
 */
long double cos_l(long double angle);


/**
 * @brief       Get sqrt value with double type 
 *
 * @param[in]   number : number to be sqrt-ed
 *
 * @return      sqrt value with double type.
 *
 * @par sample
 * @code
 *              $MATH.sqrt(2.0)
 * @endcode
 *
 * @note        
 *
 * @sa          sqrt_l()
 */
double sqrt(double number);


/**
 * @brief       Get sqrt value with long double type 
 *
 * @param[in]   number : the number to be sqrt-ed
 *
 * @return      sqrt value with long double type.
 *
 * @par sample
 * @code
 *              $MATH.sqrt_l(2.0)
 * @endcode
 *
 * @note        
 *
 * @sa          sqrt()
 */
long double sqrt_l(long double number);


/**
 * @brief       Get evaluation of a  expression with double type
 *
 * @param[in]   expression : expression for evaluated
 * @param[in]   dict       : all variables in expression can be looked up with. If no variables in expression, it should be omitted.
 *
 * @return      evaluated value with double type.
 *
 * @par sample
 * @code
 *              // expression without variables
 *              $MATH.eval("(500 + 10) * (700 + 30)")
 *
 *              // expression with variables
 *              $MATH.eval("pi * r * r", { pi: $MATH.pi, r: $MATH.sqrt(2) })
 * @endcode
 *
 * @note        
 *
 * @sa          eval_l()
 */
double eval(string expression [, object dict]);


/**
 * @brief       Get evaluation of a expression with long double type
 *
 * @param[in]   expression : expression for evaluated
 * @param[in]   dict       : all variables in expression can be looked up with. If no variables in expression, it should be omitted.
 *
 * @return      evaluated value with long double type.
 *
 * @par sample
 * @code
 *              // expression without variables
 *              $MATH.eval("(500 + 10) * (700 + 30)")
 *
 *              // expression with variables
 *              $MATH.eval("pi * r * r", { pi: $MATH.pi, r: $MATH.sqrt(2) })
 * @endcode
 *
 * @note        
 *
 * @sa          eval()
 */
long double eval_l(string expression [, object dict]);

/** @} end of lv_math */
/** @} end of loadable_vars */
