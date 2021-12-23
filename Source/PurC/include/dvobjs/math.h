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
 * @return      sin value with double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.sin(1.0)
 * @endcode
 *
 * @note        The angle is given in RADIANS.
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_FEINVALID
 *
 * @sa          sin_l()
 */
double sin(<number | longint | ulongint | long double> angle);


/**
 * @brief       Get sin value with long double type
 *
 * @param[in]   angle : the angle given in radians
 *
 * @return      sin value with long double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.sin_l(1.0)
 * @endcode
 *
 * @note        The angle is given in RADIANS.
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_FEINVALID
 *
 * @sa          sin()
 */
long double sin_l(<number | longint | ulongint | long double> angle);

/**
 * @brief       Get arcsin value with double type
 *
 * @param[in]   value: sin value of wanted angle
 *
 * @return      angle with double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.asin(0.5)
 * @endcode
 *
 * @note        The returned angle is given in RADIANS.
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_FEINVALID
 *
 * @sa          asin_l()
 */
double asin(<number | longint | ulongint | long double> value);


/**
 * @brief       Get arcsin value with double type
 *
 * @param[in]   value: sin value of wanted angle
 *
 * @return      angle with long double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.asin_l(1.0)
 * @endcode
 *
 * @note        The returned angle is given in RADIANS.
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_FEINVALID
 *
 * @sa          asin()
 */
long double asin_l(<number | longint | ulongint | long double> value);


/**
 * @brief       Get cos value with double type
 *
 * @param[in]   angle : the angle given in radians
 *
 * @return      cos value with double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.cos(1.0)
 * @endcode
 *
 * @note        The angle is given in RADIANS.
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_FEINVALID
 *
 * @sa          cos_l()
 */
double cos(<number | longint | ulongint | long double> angle);


/**
 * @brief       Get cos value with long double type 
 *
 * @param[in]   angle : the angle given in radians
 *
 * @return      cos value with long double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.cos_l(1.0)
 * @endcode
 *
 * @note        The angle is given in RADIANS.
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_FEINVALID
 *
 * @sa          cos()
 */
long double cos_l(<number | longint | ulongint | long double> angle);


/**
 * @brief       Get arccos value with double type
 *
 * @param[in]   value: cos value of wanted angle
 *
 * @return      angle with double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.acos(0.5)
 * @endcode
 *
 * @note        The returned angle is given in RADIANS.
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_FEINVALID
 *
 * @sa          acos_l()
 */
double acos(<number | longint | ulongint | long double> value);


/**
 * @brief       Get arccos value with double type
 *
 * @param[in]   value: cos value of wanted angle
 *
 * @return      angle with long double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.acos_l(1.0)
 * @endcode
 *
 * @note        The returned angle is given in RADIANS.
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_FEINVALID
 *
 * @sa          acos()
 */
long double acos_l(<number | longint | ulongint | long double> value);

/**
 * @brief       Get tan value with double type
 *
 * @param[in]   angle : the angle given in radians
 *
 * @return      cos value with double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.tan(1.0)
 * @endcode
 *
 * @note        The angle is given in RADIANS.
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_FEINVALID
 *              PURC_ERROR_OVERFLOW
 *
 * @sa          tan_l()
 */
double tan(<number | longint | ulongint | long double> angle);


/**
 * @brief       Get tan value with long double type
 *
 * @param[in]   angle : the angle given in radians
 *
 * @return      cos value with long double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.tan_l(1.0)
 * @endcode
 *
 * @note        The angle is given in RADIANS.
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_FEINVALID
 *              PURC_ERROR_OVERFLOW
 *
 * @sa          tan()
 */
long double tan_l(<number | longint | ulongint | long double> angle);

/**
 * @brief       Get arctan value with double type
 *
 * @param[in]   value: tan value of wanted angle
 *
 * @return      angle with double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.atan(0.5)
 * @endcode
 *
 * @note        The returned angle is given in RADIANS.
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_FEINVALID
 *
 * @sa          atan_l()
 */
double atan(<number | longint | ulongint | long double> value);


/**
 * @brief       Get arctan value with double type
 *
 * @param[in]   value: tan value of wanted angle
 *
 * @return      angle with long double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.atan_l(1.0)
 * @endcode
 *
 * @note        The returned angle is given in RADIANS.
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_FEINVALID
 *
 * @sa          atan()
 */
long double atan_l(<number | longint | ulongint | long double> value);

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
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_FEINVALID
 *
 * @sa          sqrt_l()
 */
double sqrt(<number | longint | ulongint | long double> number);


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
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_FEINVALID
 *
 * @sa          sqrt()
 */
long double sqrt_l(<number | longint | ulongint | long double> number);


/**
 * @brief       Compute the floating-point remainder of dividing value1 by value2.
 *
 * @param[in]   value1: divider
 * @param[in]   value2: dividend
 *
 * @return      remainder with double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.fmod(0.5, 0.2)
 * @endcode
 *
 * @note
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_FEINVALID
 *
 * @sa          fmod_l()
 */
double fmod(<number | longint | ulongint | long double> value1, <number | longintlong | ulongint | long double> value2);


/**
 * @brief       Compute the floating-point remainder of dividing value1 by value2.
 *
 * @param[in]   value1: divider
 * @param[in]   value2: dividend
 *
 * @return      remainder with long double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.fmod_l(1.0, 2.0)
 * @endcode
 *
 * @note
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_FEINVALID
 *
 * @sa          fmod()
 */
long double fmod_l(<number | longint | ulongint | long double> value1, <number | longintlong | ulongint | long double> value2);

/**
 * @brief       Get absolute value
 *
 * @param[in]   value: input value
 *
 * @return      absolute value with double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.fabs(0.5)
 * @endcode
 *
 * @note
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_FEINVALID
 *
 * @sa          fabs_l()
 */
double fabs(<number | longint | ulongint | long double> value);


/**
 * @brief       Get absolute value
 *
 * @param[in]   value: input value
 *
 * @return      absolute value with long double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.fabs_l(-1.0)
 * @endcode
 *
 * @note
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_FEINVALID
 *
 * @sa          fabs()
 */
long double fabs_l(<number | longint | ulongint | long double> value);

/**
 * @brief       Get natural logarithmic value
 *
 * @param[in]   value: input value
 *
 * @return      natural logarithmic value with double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.log(0.5)
 * @endcode
 *
 * @note
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_DIVBYZERO
 *              PURC_ERROR_FEINVALID
 *
 * @sa          log_l()
 */
double log(<number | longint | ulongint | long double> value);


/**
 * @brief       Get natural logarithmic value
 *
 * @param[in]   value: input value
 *
 * @return      natural logarithmic value with long double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.log_l(1.0)
 * @endcode
 *
 * @note
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_DIVBYZERO
 *              PURC_ERROR_FEINVALID
 *
 * @sa          log()
 */
long double log_l(<number | longint | ulongint | long double> value);

/**
 * @brief       Get base-10 logarithmic value
 *
 * @param[in]   value: input value
 *
 * @return      base-10 logarithmic value with double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.log10(0.5)
 * @endcode
 *
 * @note
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_DIVBYZERO
 *              PURC_ERROR_FEINVALID
 *
 * @sa          log10_l()
 */
double log10(<number | longint | ulongint | long double> value);


/**
 * @brief       Get base-10 logarithmic value
 *
 * @param[in]   value: input value
 *
 * @return      base-10 logarithmic value with long double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.log10_l(1.0)
 * @endcode
 *
 * @note
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_DIVBYZERO
 *              PURC_ERROR_FEINVALID
 *
 * @sa          log10()
 */
long double log10_l(<number | longint | ulongint | long double> value);

/**
 * @brief       Power function
 *
 * @param[in]   value1: basic number
 * @param[in]   value2: exponent
 *
 * @return      power value with double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.pow(0.5, 0.2)
 * @endcode
 *
 * @note
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_OVERFLOW
 *              PURC_ERROR_UNDERFLOW
 *              PURC_ERROR_DIVBYZERO
 *              PURC_ERROR_FEINVALID
 *
 * @sa          pow_l()
 */
double pow(<number | longint | ulongint | long double> value1, <number | longintlong | ulongint | long double> value2);


/**
 * @brief       Power function
 *
 * @param[in]   value1: basic number
 * @param[in]   value2: exponent
 *
 * @return      power value with long double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.pow_l(1.0, 2.0)
 * @endcode
 *
 * @note
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_OVERFLOW
 *              PURC_ERROR_UNDERFLOW
 *              PURC_ERROR_DIVBYZERO
 *              PURC_ERROR_FEINVALID
 *
 * @sa          pow()
 */
long double pow_l(<number | longint | ulongint | long double> value1, <number | longintlong | ulongint | long double> value2);

/**
 * @brief       Base-e power function
 *
 * @param[in]   value: exponent
 *
 * @return      power value with double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.exp(0.2)
 * @endcode
 *
 * @note
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_OVERFLOW
 *              PURC_ERROR_UNDERFLOW
 *              PURC_ERROR_DIVBYZERO
 *              PURC_ERROR_FEINVALID
 *
 * @sa          exp_l()
 */
double exp(<number | longint | ulongint | long double> value);


/**
 * @brief       Base-e power functions
 *
 * @param[in]   value: exponent
 *
 * @return      power value with long double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.exp_l(2.0)
 * @endcode
 *
 * @note
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_OVERFLOW
 *              PURC_ERROR_UNDERFLOW
 *              PURC_ERROR_DIVBYZERO
 *              PURC_ERROR_FEINVALID
 *
 * @sa          exp()
 */
long double exp_l(<number | longint | ulongint | long double> value);

/**
 * @brief       Get largest integral value not greater than argument
 *
 * @param[in]   value: input value
 *
 * @return      result value with double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.floor(0.2)
 * @endcode
 *
 * @note
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_FEINVALID
 *
 * @sa          floor_l()
 */
double floor(<number | longint | ulongint | long double> value);


/**
 * @brief       Get largest integral value not greater than argument
 *
 * @param[in]   value: input value
 *
 * @return      result value with long double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.floor_l(2.2)
 * @endcode
 *
 * @note
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_FEINVALID
 *
 * @sa          floor()
 */
long double floor_l(<number | longint | ulongint | long double> value);

/**
 * @brief       Get smallest integral value not less than argument
 *
 * @param[in]   value: input value
 *
 * @return      result value with double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.ceil(0.2)
 * @endcode
 *
 * @note
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_FEINVALID
 *
 * @sa          ceil_l()
 */
double ceil(<number | longint | ulongint | long double> value);


/**
 * @brief       Get smallest integral value not less than argument
 *
 * @param[in]   value: input value
 *
 * @return      result value with long double type, or PURC_VARIANT_INVALID when any exception occurs.
 *
 * @par sample
 * @code
 *              $MATH.ceil_l(2.2)
 * @endcode
 *
 * @note
 *
 * @exception   PURC_ERROR_ARGUMENT_MISSED
 *              PURC_ERROR_FEINVALID
 *
 * @sa          ceil()
 */
long double ceil_l(<number | longint | ulongint | long double> value);

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
