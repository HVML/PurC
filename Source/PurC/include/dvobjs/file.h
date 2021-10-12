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


/**
 * @brief       Open a file with stream method
 *
 * @param[in]   filename : the file name
 * @param[in]   mode     : open mode, just as which in fopen
 *
 * @return      A native variant.
 *
 * @par sample
 * @code
 *              $FILE.stream.open("/home/userhome/readme.txt", "r+")
 *
 * @endcode
 *
 * @note
 *
 * @sa          stream.close()  stream.seek()
 */
native stream.open(string: filename, number: mode);


/**
 * @brief       Sets the file position indicator
 *
 * @param[in]   file     : the file name
 * @param[in]   offset   : the offset relative to whence
 * @param[in]   whence   : start position, just as which in fseek
 *
 * @return      Current offset
 *
 * @par sample
 * @code
 *              $FILE.stream.seek(file, 2000, SEEK_CUR)
 *
 * @endcode
 *
 * @note
 *
 * @sa          stream.close()  stream.open()
 */
longint stream.seek(native: file, longint: offset, longint: whence);


/**
 * @brief       Close a stream
 *
 * @param[in]   file     : the file name
 *
 * @return      True for sucessfully, otherwise false
 *
 * @par sample
 * @code
 *              $FILE.stream.seek(file, 2000, SEEK_CUR)
 *
 * @endcode
 *
 * @note
 *
 * @sa          stream.close()  stream.seek()
 */
boolean stream.close(native: file);


/**
 * @brief       Read indicated bytes from a stream
 *
 * @param[in]   file     : the file name
 * @param[in]   bytes    : bytes to be read
 *
 * @return      A byte sequence variant, contains the indicated bytes
 *
 * @par sample
 * @code
 *              $FILE.stream.readbytes(file, 1024)
 *
 * @endcode
 *
 * @note
 *
 * @sa          stream.close()  stream.seek()  stream.close()  stream.readlines()
 */
bytesequence stream.readbytes(native: file, number: bytes);


/**
 * @brief       Read indicated lines from a stream
 *
 * @param[in]   file     : the file name
 * @param[in]   lines    : lines to be read
 *
 * @return      A string variant, contains the indicated lines
 *
 * @par sample
 * @code
 *              $FILE.stream.readlines(file, 10)
 *
 * @endcode
 *
 * @note
 *
 * @sa          stream.close()  stream.seek()  stream.close()  stream.readbytes()
 */
string stream.readlines(native: file, number: lines);


/**
 * @brief       Read a structure from a stream accroding to the format
 *
 * @param[in]   file     : the file name
 * @param[in]   format   : the read format
 *
 * @return      An array variant, contains the data in the structure
 *
 * @par sample
 * @code
 *              $FILE.stream.readstruct(file, "u32 s128")
 *
 * @endcode
 *
 * @note
 *
 * @sa          stream.close()  stream.seek()  stream.close()  stream.writestruct()
 */
array stream.readstruct(native: file, string: format);


/**
 * @brief       Write a structure to a stream accroding to the format
 *
 * @param[in]   file     : the file name
 * @param[in]   format   : the write format
 * @param[in]   data     : the data to be written
 *
 * @return      A number variant, indicates the number of written bytes
 *
 * @par sample
 * @code
 *              $FILE.stream.writestruct(file, "u32 s128", [128, "hello world"])
 *
 * @endcode
 *
 * @note
 *
 * @sa          stream.close()  stream.seek()  stream.close()  stream.writestruct()
 */
number stream.writestruct(native: file, string: format, array: data);

/** @} end of lv_file */
/** @} end of loadable_vars */
