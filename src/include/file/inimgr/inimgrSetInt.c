/* inimgrSetInt.c */

#include "inimgr_types.h"

int inimgrSetInt( IniUID uid, const char *section, const char *key, const long num )
{
	char value[32];
	
	value[0] = '\0';
	snprintf( value, sizeof( value ), "%ld", num );
	
	return __inimgr_set_value( (struct inimgr_params *)uid, section, key, value );
}
