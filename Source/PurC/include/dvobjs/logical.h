/**
 * @brief The description of Builtin dynamic variants.
 * \defgroup builtin_vars Builtin Variables
 * @{
 */

/**
 * \defgroup bv_logical $L
 * @{
 */

/**
 * @brief       Get logical invert 
 *
 * @param[in]   value : the value to be inverted
 *
 * @return      A boolean variant with logical invert.
 *
 * @par sample
 * @code
 *              $L.not(true)
 *
 * @endcode
 * @note
 *
 * @sa          and()    or()    xor() 
 */
boolean not(<any> value);


/**
 * @brief       Get logical AND
 *
 * @param[in]   value1 : the value to be AND operation
 * @param[in]   value2 : the value to be AND operation
 * @param[in]   value3 : the value to be AND operation
 *
 * @return      A boolean variant with logical AND.
 *
 * @par sample
 * @code
 *              $L.and(true, 5, 0, "hello world")
 *
 * @endcode
 * @note
 *
 * @sa          not()  or()  xor()
 */
boolean and(<any> value1, <any> value2 [, <any> value3 [, .....]]);


/**
 * @brief       Get logical OR
 *
 * @param[in]   value : the value to be OR operation
 *
 * @return      A boolean variant with logical OR.
 *
 * @par sample
 * @code
 *              $L.or(true, 5, 0, "hello world")
 *
 * @endcode
 * @note
 *
 * @sa          not()  and()  xor()
 */
boolean or(<any> value1, <any> value2 [, <any> value3 [, .....]]);


/**
 * @brief       Get logical XOR 
 *
 * @param[in]   value1 : the value to be XOR operation
 * @param[in]   value2 : the value to be XOR operation
 *
 * @return      A boolean variant with logical XOR.
 *
 * @par sample
 * @code
 *              $L.xor(true, 5)
 *
 * @endcode
 * @note
 *
 * @sa          not()  and()  or() 
 */
boolean xor(<any> value1, <any> value2);


/**
 * @brief       Compare two variant, whether they are equal. 
 *
 * @param[in]   value1 : the value to be compared
 * @param[in]   value2 : the value to be compared
 *
 * @return      A boolean variant, true for successful, otherwise false.
 *
 * @par sample
 * @code
 *              $L.eq($MATH.sin(2), 5)
 *
 * @endcode
 * @note
 *
 * @sa          eq()  ne()  gt()  ge()  lt()  le() 
 */
boolean eq(<any> value1, <any> value2);


/**
 * @brief       Compare two variant, whether they are not equal. 
 *
 * @param[in]   value1 : the value to be compared
 * @param[in]   value2 : the value to be compared
 *
 * @return      A boolean variant, true for successful, otherwise false.
 *
 * @par sample
 * @code
 *              $L.ne("1", 1)
 *
 * @endcode
 * @note
 *
 * @sa          eq()  gt()  ge()  lt()  le() 
 */
boolean ne(<any> value1, <any> value2);


/**
 * @brief       Compare two variant, whether value1 is greater than value2 
 *
 * @param[in]   value1 : the value to be compared
 * @param[in]   value2 : the value to be compared
 *
 * @return      A boolean variant, true for successful, otherwise false.
 *
 * @par sample
 * @code
 *              $L.gt($MATH.sin(2), 0.2)
 *
 * @endcode
 * @note
 *
 * @sa          eq()  ne()  ge()  lt()  le() 
 */
boolean gt(<any> value1, <any> value2);


/**
 * @brief       Compare two variant, whether value1 is more than or equal to value2 
 *
 * @param[in]   value1 : the value to be compared
 * @param[in]   value2 : the value to be compared
 *
 * @return      A boolean variant, true for successful, otherwise false.
 *
 * @par sample
 * @code
 *              $L.ge($MATH.sin(2), 0.2)
 *
 * @endcode
 * @note
 *
 * @sa          eq()  ne()  gt()  lt()  le() 
 */
boolean ge(<any> value1, <any> value2);


/**
 * @brief       Compare two variant, whether value1 is less than value2 
 *
 * @param[in]   value1 : the value to be compared
 * @param[in]   value2 : the value to be compared
 *
 * @return      A boolean variant, true for successful, otherwise false.
 *
 * @par sample
 * @code
 *              $L.lt($MATH.sin(2), 0.2)
 *
 * @endcode
 * @note
 *
 * @sa          eq()  ne()  ge()  gt()  le() 
 */
boolean lt(<any> value1, <any> value2);


/**
 * @brief       Compare two variant, whether value1 is less than or equal to value2 
 *
 * @param[in]   value1 : the value to be compared
 * @param[in]   value2 : the value to be compared
 *
 * @return      A boolean variant, true for successful, otherwise false.
 *
 * @par sample
 * @code
 *              $L.le($MATH.sin(2), 0.2)
 *
 * @endcode
 * @note
 *
 * @sa          eq()  ne()  ge()  gt()  lt() 
 */
boolean le(<any> value1, <any> value2);


/**
 * @brief       Compare two string, whether value1 is equal to value2 
 *
 * @param[in]   option : compare option, it can be\n
 *                  caseless | case | wildcard | reg
 * @param[in]   value1 : the value to be compared, or wildcard | reg, if first parameter is wildcard | reg. 
 * @param[in]   value2 : the value to be compared
 *
 * @return      A boolean variant, true for successful, otherwise false.
 *
 * @par sample
 * @code
 *              $L.streq("wildcard", "*world", "hello world")
 *
 * @endcode
 * @note
 *
 * @sa          strne()  strge()  strgt()  strlt()  strle()
 */
boolean streq(string option, <any> value1, <any> value2);


/**
 * @brief       Compare two string, whether value1 is not equal to value2 
 *
 * @param[in]   option : compare option, it can be\n
 *                  caseless | case | wildcard | reg
 * @param[in]   value1 : the value to be compared, or wildcard | reg, if first parameter is wildcard | reg. 
 * @param[in]   value2 : the value to be compared
 *
 * @return      A boolean variant, true for successful, otherwise false.
 *
 * @par sample
 * @code
 *              $L.strne("wildcard", "*world", "hello world")
 *
 * @endcode
 * @note
 *
 * @sa          streq()  strge()  strgt()  strlt()  strle()
 */
boolean strne(string option, <any> value1, <any> value2);


/**
 * @brief       Compare two string, whether value1 is greater than value2 
 *
 * @param[in]   option : compare option, it can be\n
 *                  caseless | case
 * @param[in]   value1 : the value to be compared 
 * @param[in]   value2 : the value to be compared
 *
 * @return      A boolean variant, true for successful, otherwise false.
 *
 * @par sample
 * @code
 *              $L.strgt("wildcard", "New York", "hello world")
 *
 * @endcode
 * @note
 *
 * @sa          streq()  strne()  strge()  strlt()  strle()
 */
boolean strgt(string option, <any> value1, <any> value2);


/**
 * @brief       Compare two string, whether value1 is greater than or equal to value2 
 *
 * @param[in]   option : compare option, it can be\n
 *                  caseless | case
 * @param[in]   value1 : the value to be compared 
 * @param[in]   value2 : the value to be compared
 *
 * @return      A boolean variant, true for successful, otherwise false.
 *
 * @par sample
 * @code
 *              $L.strge("wildcard", "New York", "hello world")
 *
 * @endcode
 * @note
 *
 * @sa          streq()  strne()  strgt()  strlt()  strle()
 */
boolean strge(string option, <any> value1, <any> value2);


/**
 * @brief       Compare two string, whether value1 is less than value2 
 *
 * @param[in]   option : compare option, it can be\n
 *              caseless | case
 * @param[in]   value1 : the value to be compared 
 * @param[in]   value2 : the value to be compared
 *
 * @return      A boolean variant, true for successful, otherwise false.
 *
 * @par sample
 * @code
 *              $L.strlt("wildcard", "New York", "hello world")
 *
 * @endcode
 * @note
 *
 * @sa          streq()  strne()  strge()  strgt()  strle()
 */
boolean strlt(string option, <any> value1, <any> value2);


/**
 * @brief       Compare two string, whether value1 is less than or equal to value2 
 *
 * @param[in]   option : compare option, it can be\n
 *                  caseless | case
 * @param[in]   value1 : the value to be compared 
 * @param[in]   value2 : the value to be compared
 *
 * @return      A boolean variant, true for successful, otherwise false.
 *
 * @par sample
 * @code
 *              $L.strle("wildcard", "New York", "hello world")
 *
 * @endcode
 * @note
 *
 * @sa          streq()  strne()  strgt()  strge()  strlt()
 */
boolean strle(string option, <any> value1, <any> value2);


/**
 * @brief       Get evaluation of an expression with boolean type
 *
 * @param[in]   expression : expression for evaluated
 * @param[in]   dict       : all variables in expression can be looked up with. If no variables in expression, it should be omitted.
 *
 * @return      evaluated value with boolean type.
 *
 * @par sample
 * @code
 *              $L.eval("x > y && y > z || b", { x: 2, y: 1, z: 0, b: $L.streq("case", $a, $b) })
 * @endcode
 *
 * @note
 */
boolean eval(string expression [, object dict]);


/** @} end of bv_logical */
/** @} end of builtin_vars */
