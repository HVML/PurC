/**
 * @brief The description of Loadable dynamic variants.
 * \defgroup loadable_vars Loadable Variables
 * @{
 */

/**
 * \defgroup lv_file $FILE
 * @{
 */


/**
 * @brief       Get the indicated lines of a text file from file head
 *
 * @param[in]   filename : the file name
 * @param[in]   lines    : indicated lines
 *
 * @return      A string variant, which contains indicated lines.
 *
 * @par sample
 * @code
 *              // get all lines
 *              $FILE.txt.head("/home/userhome/readme.txt")
 *
 *              // get first 5 lines
 *              $FILE.txt.head("/home/userhome/readme.txt", 5)
 *
 *              // get all lines, excepting last 5 lines
 *              $FILE.txt.head("/home/userhome/readme.txt", -5)
 * @endcode
 *
 * @note        
 *
 * @sa          txt.tail()
 */
string txt.head(string: filename [, number: lines]);


/**
 * @brief       Get the indicated lines of a text file from file tail
 *
 * @param[in]   filename : the file name
 * @param[in]   lines    : indicated lines
 *
 * @return      A string variant, which contains indicated lines.
 *
 * @par sample
 * @code
 *              // get all lines
 *              $FILE.txt.tail("/home/userhome/readme.txt")
 *
 *              // get last 5 lines
 *              $FILE.txt.tail("/home/userhome/readme.txt", 5)
 *
 *              // get all lines, excepting first 5 lines
 *              $FILE.txt.tail("/home/userhome/readme.txt", -5)
 * @endcode
 *
 * @note        
 *
 * @sa          txt.head()
 */
string txt.tail(string: filename [, number: lines]);


/**
 * @brief       Get the indicated bytes of a binary file from file head
 *
 * @param[in]   filename : the file name
 * @param[in]   bytes    : indicated bytes
 *
 * @return      A byte sequence variant, which contains indicated bytes.
 *
 * @par sample
 * @code
 *              // get all file
 *              $FILE.bin.head("/home/userhome/exe")
 *
 *              // get first 5 bytes
 *              $FILE.bin.head("/home/userhome/exe", 5)
 *
 *              // get all file, excepting last 5 bytes
 *              $FILE.bin.head("/home/userhome/exe", -5)
 * @endcode
 *
 * @note        
 *
 * @sa          bin.tail()
 */
bytesequence bin.head(string: filename [, number: bytes]);


/**
 * @brief       Get the indicated bytes of a binary file from file tail 
 *
 * @param[in]   filename : the file name
 * @param[in]   bytes    : indicated bytes
 *
 * @return      A byte sequence variant, which contains indicated bytes.
 *
 * @par sample
 * @code
 *              // get all file
 *              $FILE.bin.tail("/home/userhome/exe")
 *
 *              // get last 5 bytes
 *              $FILE.bin.tail("/home/userhome/exe", 5)
 *
 *              // get all file, excepting first 5 bytes
 *              $FILE.bin.tail("/home/userhome/exe", -5)
 * @endcode
 *
 * @note        
 *
 * @sa          bin.head()
 */
bytesequence bin.tail(string: filename [, number: bytes]);


/** @} end of lv_file */
/** @} end of loadable_vars */
