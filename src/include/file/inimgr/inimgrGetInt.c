/* inimgrGetInt.c */

#include "inimgr_types.h"

bool inimgrGetInt( IniUID uid, const char *section, const char *key, int *var )
{
	char *value = __inimgr_get_value( (struct inimgr_params *)uid, section, key );
	
	if( value ){
		*var = strtol( value, NULL, 10 );
		return true;
	} else{
		return false;
	}
}
