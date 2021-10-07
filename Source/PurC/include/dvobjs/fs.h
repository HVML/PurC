/**
 * @brief The description of Loadable dynamic variants.
 * \defgroup loadable_vars Loadable Variables
 * @{
 */

/**
 * \defgroup lv_fs $FS
 * @{
 */


/**
 * @brief       List all files and sub-directies in a path 
 *
 * @param[in]   path    : the directory to be listed
 * @param[in]   filters : the list of semicolon separated name filters
 *
 * @return      A array variant, each element in it is object variant, contains the information of the file or sub-directories.
 *
 * @par sample
 * @code
 *              $FS.list("/home/usr/local", "*.txt; *.md")
 * @endcode
 *
 * @note        The object in array, contains the items as below:
 * @code
 *      {
 *          "name"       : <string: name of the file (directory entry)>,
 *          "dev"        : <number: ID of device containing file>,
 *          "inode"      : <number: inode number>,
 *          "type"       : <string: file type like 'd', 'b', 's', ...>,
 *          "mode"       : <bytesequece: file mode>,
 *          "mode_str"   : <string: file mode like `rwxrwxr-x`>,
 *          "nlink"      : <number: number of hard links>,
 *          "uid"        : <number: the user ID of owner>,
 *          "gid"        : <number: the group ID of owner>,
 *          "rdev_major" : <number: the major device ID if it is a special file>,
 *          "rdev_minor" : <number: the minor device ID if it is a special file>,
 *          "size"       : <number: total size in bytes>,
 *          "blksize"    : <number: block size for filesystem I/O>,
 *          "blocks"     : <number: Number of 512B blocks allocated>,
 *          "atime"      : <number: time of last acces>,
 *          "mtime"      : <number: time of last modification>,
 *          "ctime"      : <number: time of last status change>
 *      }
 *
 * @sa          list_prt()
 */
array list(string: path [, string: filters]);


/**
 * @brief       List all files and sub-directies in a path 
 *
 * @param[in]   path    : the directory to be listed
 * @param[in]   filters : the list of semicolon separated name filters
 * @param[in]   order   : the information order in a string, it can be:\n
                        [mode || nlink || uid || gid || size || blksize || atime || ctime || mtime || name] | all | default
 *
 * @return      A array variant, each element in it is string variant, contains the information of the file or sub-directories.
 *
 * @par sample
 * @code
 *              $FS.list_ptr("/home/usr/local", "*.txt; *.md", "mode nlink uid gid size")
 * @endcode
 *
 * @note        The information in a string, just like Linux command ls
 * @sa          list()
 */
array list_prt(string: path [, string filters [, string order]]);


/**
 * @brief       Create a new directory 
 *
 * @param[in]   path    : the directory to be created 
 *
 * @return      A boolean variant, true for successful, otherwise false.
 *
 * @par sample
 * @code
 *              $FS.mkdir("/home/user_home/workspace")
 * @endcode
 *
 * @note
 * @sa          rmdir()  rm()  unlink()
 */
boolean mkdir(string: path);


/**
 * @brief       Remove an empty directory 
 *
 * @param[in]   path    : the directory to be removed 
 *
 * @return      A boolean variant, true for successful, otherwise false.
 *
 * @par sample
 * @code
 *              $FS.rmdir("/home/user_home/workspace")
 * @endcode
 *
 * @note        If the directory is not empty, nothing to do, but return false.
 * @sa          mkdir()  rm()  unlink()
 */
boolean rmdir(string: path);


/**
 * @brief       Remove all files and sub-directories in a directory
 *
 * @param[in]   path    : the directory to be removed 
 *
 * @return      A boolean variant, true for successful, otherwise false.
 *
 * @par sample
 * @code
 *              $FS.rm("/home/user_home/workspace")
 * @endcode
 *
 * @note
 * @sa          mkdir()  rmdir()  unlink()
 */
boolean rm(string: path);


/**
 * @brief       Remove a file
 *
 * @param[in]   path    : the file to be removed 
 *
 * @return      A boolean variant, true for successful, otherwise false.
 *
 * @par sample
 * @code
 *              $FS.unlink("/home/user_home/workspace/hello.java")
 * @endcode
 *
 * @note
 * @sa          mkdir()  rmdir()  rm()
 */
boolean unlink(string: path);


/**
 * @brief       Modified the file access time and modified time
 *
 * @param[in]   path    : the file to be touched 
 *
 * @return      A boolean variant, true for successful, otherwise false.
 *
 * @par sample
 * @code
 *              $FS.touch("/home/user_home/workspace")
 * @endcode
 *
 * @note
 * @sa          mkdir()  rmdir()  unlink()
 */
boolean touch(string: path);



/** @} end of lv_fs */
/** @} end of loadable_vars */
