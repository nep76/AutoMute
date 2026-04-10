/* inimgrGetString.c */

#include "inimgr_types.h"

int inimgrGetString( IniUID uid, const char *section, const char *key, char *buf, size_t bufsize )
{
	char *value = __inimgr_get_value( (struct inimgr_params *)uid, section, key );
	
	if( value ){
		strutilCopy( buf, value, bufsize );
		return strlen( buf );
	} else {
		return -1;
	}
}

