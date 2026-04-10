/* inimgrSetBool.c */

#include "inimgr_types.h"

int inimgrSetBool( IniUID uid, const char *section, const char *key, const bool on )
{
	char value[4];
	
	if( on ){
		strcpy( value, "On" );
	} else{
		strcpy( value, "Off" );
	}
	
	return __inimgr_set_value( (struct inimgr_params *)uid, section, key, value );
}
