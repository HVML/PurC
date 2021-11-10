#ifndef MC__EDIT_ETAGS_H
#define MC__EDIT_ETAGS_H 1

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

typedef struct etags_hash_struct
{
    char *filename;
    char *fullpath;
    char *short_define;
    long line;
} etags_hash_t;

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

void edit_get_match_keyword_cmd (WEdit * edit);

/*** inline functions ****************************************************************************/

#endif /* MC__EDIT_ETAGS_H */
