/* inimgrGetBool.c */

#include "inimgr_types.h"

bool inimgrGetBool( IniUID uid, const char *section, const char *key, bool *var )
{
	char *value = __inimgr_get_value( (struct inimgr_params *)uid, section, key );
	
	if( value ){
		strupr( value );
		if( strcasecmp( value, "On" ) == 0 ){
			*var = true;
		} else if( strcasecmp( value, "Off" ) == 0 ){
			*var = false;
		}
		return true;
	}
	
	return false;
}
