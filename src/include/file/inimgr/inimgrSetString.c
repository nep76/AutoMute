/* inimgrSetString.c */

#include "inimgr_types.h"

int inimgrSetString( IniUID uid, const char *section, const char *key, const char *str )
{
	return __inimgr_set_value( (struct inimgr_params *)uid, section, key, str );
}
